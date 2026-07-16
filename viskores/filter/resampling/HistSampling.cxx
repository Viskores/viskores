//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/density_estimate/Histogram.h>
#include <viskores/filter/entity_extraction/ThresholdPoints.h>
#include <viskores/filter/resampling/HistSampling.h>
#include <viskores/filter/resampling/worklet/HistSampling.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace filter
{
namespace resampling
{
namespace
{
viskores::cont::ArrayHandle<viskores::FloatDefault> CalculatPdf(
  viskores::Id totalPoints,
  viskores::FloatDefault sampleFraction,
  viskores::cont::ArrayHandle<viskores::Id> binCount)
{
  viskores::Id NumBins = binCount.GetNumberOfValues();
  viskores::cont::ArrayHandleIndex indexArray(NumBins);
  viskores::cont::ArrayHandle<viskores::Id> BinIndices;
  viskores::cont::Algorithm::Copy(indexArray, BinIndices);
  viskores::cont::Algorithm::SortByKey(binCount, BinIndices);

  viskores::FloatDefault remainingSamples = sampleFraction * totalPoints;
  viskores::FloatDefault remainingBins = static_cast<viskores::FloatDefault>(NumBins);
  viskores::cont::ArrayHandle<viskores::FloatDefault> targetSamples;
  targetSamples.Allocate(NumBins);

  auto binCountPortal = binCount.ReadPortal();
  auto targetWritePortal = targetSamples.WritePortal();

  for (int i = 0; i < NumBins; ++i)
  {
    viskores::FloatDefault targetNeededSamples = remainingSamples / remainingBins;
    viskores::FloatDefault curCount = static_cast<viskores::FloatDefault>(binCountPortal.Get(i));
    viskores::FloatDefault samplesTaken = viskores::Min(curCount, targetNeededSamples);
    targetWritePortal.Set(i, samplesTaken);
    remainingBins = remainingBins - 1;
    remainingSamples = remainingSamples - samplesTaken;
  }

  viskores::cont::ArrayHandle<viskores::FloatDefault> acceptanceProbsVec;
  acceptanceProbsVec.AllocateAndFill(NumBins, -1.f);

  viskores::cont::Invoker invoker;
  invoker(viskores::worklet::AcceptanceProbsWorklet{},
          targetSamples,
          BinIndices,
          binCount,
          acceptanceProbsVec);
  return acceptanceProbsVec;
}

} // anonymous namespace

viskores::cont::DataSet HistSampling::DoExecute(const viskores::cont::DataSet& input)
{
  if (!this->InExecutePartitions)
  {
    //computing histogram based on input
    viskores::filter::density_estimate::Histogram histogram;
    histogram.SetNumberOfBins(this->NumberOfBins);
    histogram.SetActiveField(this->GetActiveFieldName());
    auto histogramOutput = histogram.Execute(input);
    viskores::cont::ArrayHandle<viskores::Id> binCountArray;
    viskores::cont::ArrayCopyShallowIfPossible(
      histogramOutput.GetField(histogram.GetOutputFieldName()).GetData(), binCountArray);
    viskores::Id totalPoints = input.GetNumberOfPoints();
    //computing pdf
    this->Probabilities = CalculatPdf(totalPoints, this->SampleFraction, binCountArray);
    this->HistogramMinimum = histogram.GetComputedRange().Min;
    this->HistogramBinDelta = histogram.GetBinDelta();
  }

  // use the acceptance probabilities and random array to create 0-1 array
  // generating random array between 0 to 1
  viskores::cont::ArrayHandle<viskores::Int8> outputArray;
  auto resolveType = [&](const auto& concrete)
  {
    viskores::Id NumFieldValues = concrete.GetNumberOfValues();
    auto randArray = viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(
      NumFieldValues, { this->GetSeed() });
    viskores::worklet::DispatcherMapField<viskores::worklet::LookupWorklet>(
      viskores::worklet::LookupWorklet{
        this->NumberOfBins, this->HistogramMinimum, this->HistogramBinDelta })
      .Invoke(concrete, outputArray, this->Probabilities, randArray);
  };
  const auto& inField = this->GetFieldFromDataSet(input);
  this->CastAndCallScalarField(inField, resolveType);

  viskores::cont::DataSet sampledDataSet =
    this->CreateResultField(input, "ifsampling", inField.GetAssociation(), outputArray);
  viskores::filter::entity_extraction::ThresholdPoints threshold;
  threshold.SetActiveField("ifsampling");
  threshold.SetCompactPoints(true);
  threshold.SetThresholdAbove(0.5);
  // filter out the results with zero in it
  viskores::cont::DataSet thresholdDataSet = threshold.Execute(sampledDataSet);
  if (!this->InExecutePartitions)
  {
    this->Probabilities.ReleaseResources();
  }
  return thresholdDataSet;
}

viskores::cont::PartitionedDataSet HistSampling::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  if (input.GetGlobalNumberOfPartitions() == 0)
  {
    return this->Filter::DoExecutePartitions(input);
  }

  this->PreExecute(input);
  auto result = this->Filter::DoExecutePartitions(input);
  this->PostExecute();
  return result;
}

void HistSampling::PreExecute(const viskores::cont::PartitionedDataSet& input)
{
  viskores::filter::density_estimate::Histogram histogram;
  histogram.SetNumberOfBins(this->NumberOfBins);
  histogram.SetActiveField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
  histogram.SetThreadsPerCPU(this->GetThreadsPerCPU());
  histogram.SetThreadsPerGPU(this->GetThreadsPerGPU());
  histogram.SetRunMultiThreadedFilter(this->GetRunMultiThreadedFilter());
  auto histogramOutput = histogram.Execute(input);

  viskores::cont::ArrayHandle<viskores::Id> binCountArray;
  viskores::cont::ArrayCopyShallowIfPossible(
    histogramOutput.GetPartition(0).GetField(histogram.GetOutputFieldName()).GetData(),
    binCountArray);
  viskores::Id totalPoints = viskores::cont::Algorithm::Reduce(binCountArray, viskores::Id{ 0 });
  this->Probabilities = CalculatPdf(totalPoints, this->SampleFraction, binCountArray);
  this->HistogramMinimum = histogram.GetComputedRange().Min;
  this->HistogramBinDelta = histogram.GetBinDelta();
  this->InExecutePartitions = true;
}

void HistSampling::PostExecute()
{
  this->InExecutePartitions = false;
  this->Probabilities.ReleaseResources();
}

} // namespace resampling
} // namespace filter
} // namespace viskores
