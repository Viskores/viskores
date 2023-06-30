//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/entity_extraction/ThresholdPoints.h>
#include <vtkm/filter/resampling/HistSampling.h>
#include <vtkm/worklet/WorkletMapField.h>
namespace
{

struct CreateFieldValueWorklet : public vtkm::worklet::WorkletMapField
{
  vtkm::Id SizePerDim;
  VTKM_CONT CreateFieldValueWorklet(vtkm::Id sizePerDim)
    : SizePerDim(sizePerDim)
  {
  }
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1, InputIndex);
  template <typename OutPutType>
  VTKM_EXEC void operator()(OutPutType& val, vtkm::Id idx) const
  {
    vtkm::Id x = idx % this->SizePerDim;
    vtkm::Id y = (idx / this->SizePerDim) % this->SizePerDim;
    vtkm::Id z = idx / this->SizePerDim / this->SizePerDim;
    vtkm::FloatDefault center = static_cast<vtkm::FloatDefault>((1.0 * this->SizePerDim) / 2.0);
    vtkm::FloatDefault v = vtkm::Pow((static_cast<vtkm::FloatDefault>(x) - center), 2) +
      vtkm::Pow((static_cast<vtkm::FloatDefault>(y) - center), 2) +
      vtkm::Pow((static_cast<vtkm::FloatDefault>(z) - center), 2);
    if (v < 0.5)
    {
      val = static_cast<vtkm::FloatDefault>(10.0);
      return;
    }
    val = static_cast<vtkm::FloatDefault>(10.0 / vtkm::Sqrt(v));
  };
};

void TestHistSamplingSingleBlock()
{
  //creating data set for testing
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id sizePerDim = 20;
  constexpr vtkm::Id3 dimensions(sizePerDim, sizePerDim, sizePerDim);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  //creating data set for testing
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray;
  scalarArray.Allocate(sizePerDim * sizePerDim * sizePerDim);
  vtkm::cont::Invoker invoker;
  invoker(CreateFieldValueWorklet{ sizePerDim }, scalarArray);
  dataSet.AddPointField("scalarField", scalarArray);

  //calling filter
  using AsscoType = vtkm::cont::Field::Association;
  vtkm::filter::resampling::HistSampling histsample;
  histsample.SetNumberOfBins(10);
  histsample.SetActiveField("scalarField", AsscoType::Points);
  auto outputDataSet = histsample.Execute(dataSet);

  //checking data sets to make sure all rared region are sampled
  //are there better way to test it?
  vtkm::filter::entity_extraction::ThresholdPoints threshold;
  threshold.SetActiveField("scalarField");
  threshold.SetCompactPoints(true);
  threshold.SetThresholdAbove(9.9);
  vtkm::cont::DataSet thresholdDataSet = threshold.Execute(outputDataSet);
  //There are 7 points that have the scalar value of 10
  VTKM_TEST_ASSERT(thresholdDataSet.GetField("scalarField").GetNumberOfValues() == 7);
}

void TestHistSampling()
{
  TestHistSamplingSingleBlock();
} // TestHistSampling

}

int UnitTestHistSampling(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestHistSampling, argc, argv);
}
