//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/density_estimate/Histogram.h>
#include <vtkm/filter/entity_extraction/ThresholdPoints.h>
#include <vtkm/filter/resampling/HistSampling.h>
#include <vtkm/filter/resampling/worklet/HistSampling.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace filter
{
namespace resampling
{
namespace
{
vtkm::cont::ArrayHandle<vtkm::FloatDefault> CalculatPdf(vtkm::Id totalPoints,
                                                        vtkm::FloatDefault sampleFraction,
                                                        vtkm::cont::ArrayHandle<vtkm::Id> binCount)
{
  vtkm::Id NumBins = binCount.GetNumberOfValues();
  vtkm::cont::ArrayHandleIndex indexArray(NumBins);
  vtkm::cont::ArrayHandle<vtkm::Id> BinIndices;
  vtkm::cont::Algorithm::Copy(indexArray, BinIndices);
  vtkm::cont::Algorithm::SortByKey(binCount, BinIndices);

  vtkm::FloatDefault remainingSamples = sampleFraction * totalPoints;
  vtkm::FloatDefault remainingBins = static_cast<vtkm::FloatDefault>(NumBins);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> targetSamples;
  targetSamples.Allocate(NumBins);

  auto binCountPortal = binCount.ReadPortal();
  auto targetWritePortal = targetSamples.WritePortal();

  for (int i = 0; i < NumBins; ++i)
  {
    vtkm::FloatDefault targetNeededSamples = remainingSamples / remainingBins;
    vtkm::FloatDefault curCount = static_cast<vtkm::FloatDefault>(binCountPortal.Get(i));
    vtkm::FloatDefault samplesTaken = vtkm::Min(curCount, targetNeededSamples);
    targetWritePortal.Set(i, samplesTaken);
    remainingBins = remainingBins - 1;
    remainingSamples = remainingSamples - samplesTaken;
  }

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> acceptanceProbsVec;
  acceptanceProbsVec.AllocateAndFill(NumBins, -1.f);

  vtkm::cont::Invoker invoker;
  invoker(vtkm::worklet::AcceptanceProbsWorklet{},
          targetSamples,
          BinIndices,
          binCount,
          acceptanceProbsVec);
  return acceptanceProbsVec;
}

} // anonymous namespace

vtkm::cont::DataSet HistSampling::DoExecute(const vtkm::cont::DataSet& input)
{
  //computing histogram based on input
  vtkm::filter::density_estimate::Histogram histogram;
  histogram.SetNumberOfBins(this->NumberOfBins);
  histogram.SetActiveField(this->GetActiveFieldName());
  auto histogramOutput = histogram.Execute(input);
  vtkm::cont::ArrayHandle<vtkm::Id> binCountArray;
  vtkm::cont::ArrayCopyShallowIfPossible(
    histogramOutput.GetField(histogram.GetOutputFieldName()).GetData(), binCountArray);
  vtkm::Id totalPoints = input.GetNumberOfPoints();
  //computing pdf
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> probArray;
  probArray = CalculatPdf(totalPoints, this->SampleFraction, binCountArray);
  // use the acceptance probabilities and random array to create 0-1 array
  // generating random array between 0 to 1
  vtkm::cont::ArrayHandle<vtkm::Int8> outputArray;
  auto resolveType = [&](const auto& concrete) {
    vtkm::Id NumFieldValues = concrete.GetNumberOfValues();
    auto randArray = vtkm::cont::ArrayHandleRandomUniformReal<vtkm::FloatDefault>(
      NumFieldValues, { this->GetSeed() });
    vtkm::worklet::DispatcherMapField<vtkm::worklet::LookupWorklet>(
      vtkm::worklet::LookupWorklet{
        this->NumberOfBins, histogram.GetComputedRange().Min, histogram.GetBinDelta() })
      .Invoke(concrete, outputArray, probArray, randArray);
  };
  const auto& inField = this->GetFieldFromDataSet(input);
  this->CastAndCallScalarField(inField, resolveType);

  vtkm::cont::DataSet sampledDataSet =
    this->CreateResultField(input, "ifsampling", inField.GetAssociation(), outputArray);
  vtkm::filter::entity_extraction::ThresholdPoints threshold;
  threshold.SetActiveField("ifsampling");
  threshold.SetCompactPoints(true);
  threshold.SetThresholdAbove(0.5);
  // filter out the results with zero in it
  vtkm::cont::DataSet thresholdDataSet = threshold.Execute(sampledDataSet);
  return thresholdDataSet;
}

} // namespace resampling
} // namespace filter
} // namespace vtkm
