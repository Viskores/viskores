//
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <iostream>
#include <random>
#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/uncertainty/ContourUncertainUniform.h>
#include <vtkm/filter/uncertainty/ContourUncertainUniformMonteCarlo.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

namespace
{
template <typename T>
vtkm::cont::DataSet MakeContourUncertainUniformTestDataSet()
{
  const vtkm::Id3 dims(25, 25, 25);

  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet = dataSetBuilder.Create(dims);

  std::vector<T> ensemble_max;
  std::vector<T> ensemble_min;

  vtkm::IdComponent k = 0;
  vtkm::cont::ArrayHandleRandomUniformReal<vtkm::FloatDefault> randomArray(25 * 25 * 25 * 2,
                                                                           { 0xceed });
  auto portal = randomArray.ReadPortal();

  for (vtkm::Id i = 0; i < numPoints; ++i)
  {
    vtkm::FloatDefault value1 = -20 + (40 * portal.Get(k));
    vtkm::FloatDefault value2 = -20 + (40 * portal.Get(k + 1));
    ensemble_max.push_back(static_cast<T>(vtkm::Max(value1, value2)));
    ensemble_min.push_back(static_cast<T>(vtkm::Min(value1, value2)));
    k += 2;
  }

  dataSet.AddPointField("ensemble_max", ensemble_max);
  dataSet.AddPointField("ensemble_min", ensemble_min);
  return dataSet;
}

void TestUncertaintyGeneral(vtkm::FloatDefault isoValue)
{
  // Isosurface uncertainty computation using closed form solution
  vtkm::cont::DataSet input = MakeContourUncertainUniformTestDataSet<vtkm::FloatDefault>();
  vtkm::filter::uncertainty::ContourUncertainUniform filter;
  filter.SetIsoValue(isoValue);
  filter.SetCrossProbabilityName("CrossProbablity");
  filter.SetNumberNonzeroProbabilityName("NonzeroProbablity");
  filter.SetEntropyName("Entropy");
  filter.SetMinField("ensemble_min");
  filter.SetMaxField("ensemble_max");
  vtkm::cont::DataSet output = filter.Execute(input);

  // Isosurface uncertainty computation using Monte Carlo solution
  vtkm::filter::uncertainty::ContourUncertainUniformMonteCarlo filter_mc;
  filter_mc.SetIsoValue(isoValue);
  filter_mc.SetNumSample(1000);
  filter_mc.SetCrossProbabilityName("CrossProbablityMC");
  filter_mc.SetNumberNonzeroProbabilityName("NonzeroProbablityMC");
  filter_mc.SetEntropyName("EntropyMC");
  filter_mc.SetMinField("ensemble_min");
  filter_mc.SetMaxField("ensemble_max");
  vtkm::cont::DataSet output_mc = filter_mc.Execute(input);

  // Crossing Probablity field (closed form)
  vtkm::cont::Field CrossProb = output.GetField("CrossProbablity");
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> crossProbArray;
  CrossProb.GetData().AsArrayHandle(crossProbArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType CrossPortal =
    crossProbArray.ReadPortal();

  // Nonzero Value field (closed form)
  vtkm::cont::Field NonzeroProb = output.GetField("NonzeroProbablity");
  vtkm::cont::ArrayHandle<vtkm::Id> NonzeroProbArray;
  NonzeroProb.GetData().AsArrayHandle(NonzeroProbArray);
  vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType NonzeroPortal = NonzeroProbArray.ReadPortal();

  // Entropy field (closed form)
  vtkm::cont::Field entropy = output.GetField("Entropy");
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> EntropyArray;
  entropy.GetData().AsArrayHandle(EntropyArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType EntropyPortal =
    EntropyArray.ReadPortal();

  // Crossing Probablity field (Marching Cube)
  vtkm::cont::Field CrossProbMC = output_mc.GetField("CrossProbablityMC");
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> crossProbMCArray;
  CrossProbMC.GetData().AsArrayHandle(crossProbMCArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType CrossMCPortal =
    crossProbMCArray.ReadPortal();

  // Nonzero Value field (Marching Cube)
  vtkm::cont::Field NonzeroMCProb = output_mc.GetField("NonzeroProbablityMC");
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> NonzeroProbMCArray;
  NonzeroMCProb.GetData().AsArrayHandle(NonzeroProbMCArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType NonzeroMCPortal =
    NonzeroProbMCArray.ReadPortal();

  // Entropy field (Marching Cube)
  vtkm::cont::Field entropyMC = output_mc.GetField("EntropyMC");
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> EntropyMCArray;
  entropyMC.GetData().AsArrayHandle(EntropyMCArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType EntropyMCPortal =
    EntropyMCArray.ReadPortal();


  // Comparision of Closed-form and Marching Cube
  for (vtkm::Id i = 0; i < crossProbArray.GetNumberOfValues(); ++i)
  {
    vtkm::FloatDefault CrossProbValue = CrossPortal.Get(i);
    vtkm::Id NonzeroProbValue = NonzeroPortal.Get(i); // long long not float
    vtkm::FloatDefault EntropyValue = EntropyPortal.Get(i);

    vtkm::FloatDefault CrossProbMCValue = CrossMCPortal.Get(i);
    vtkm::FloatDefault NonzeroProbMCValue = NonzeroMCPortal.Get(i);
    vtkm::FloatDefault EntropyMCValue = EntropyMCPortal.Get(i);

    // Maximum Entropy Difference: 8
    // Maximum Cross Probability Difference: 1
    // Maximum Nonzero Difference: 256
    VTKM_TEST_ASSERT((vtkm::Abs(CrossProbMCValue - CrossProbValue) < 0.1) ||
                       (vtkm::Abs(NonzeroProbMCValue - NonzeroProbValue) < 50) ||
                       (vtkm::Abs(EntropyMCValue - EntropyValue) < 0.5),
                     vtkm::Abs(CrossProbMCValue - CrossProbValue),
                     ' ',
                     vtkm::Abs(NonzeroProbMCValue - NonzeroProbValue) < 50,
                     ' ',
                     vtkm::Abs(EntropyMCValue - EntropyValue),
                     " No Match With Monte Carlo Sampling");
  }
}

void TestContourUncertainUniform()
{
  vtkm::FloatDefault isoValue = 0;
  TestUncertaintyGeneral(isoValue);
}
}
int UnitTestContourUncertainUniform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourUncertainUniform, argc, argv);
}
