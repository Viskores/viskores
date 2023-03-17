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
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/density_estimate/Statistics.h>
#include <vtkm/thirdparty/diy/environment.h>

namespace
{
template <typename DataSetType>
vtkm::FloatDefault getStatsFromDataSet(const DataSetType& dataset, const std::string statName)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array;
  dataset.GetField(statName).GetData().AsArrayHandle(array);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType portal = array.ReadPortal();
  vtkm::FloatDefault value = portal.Get(0);
  return value;
}

void TestStatisticsPartial()
{
  vtkm::cont::DataSet dataSet;
  constexpr vtkm::FloatDefault N = 1000;
  auto scalarArrayCounting =
    vtkm::cont::ArrayHandleCounting<vtkm::FloatDefault>(0.0f, 1.0f, static_cast<vtkm::Id>(N));
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray;
  vtkm::cont::ArrayCopy(scalarArrayCounting, scalarArray);
  dataSet.AddPointField("scalarField", scalarArray);

  using STATS = vtkm::filter::density_estimate::Statistics;
  STATS statisticsFilter;
  using AsscoType = vtkm::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  vtkm::cont::DataSet resultDataSet = statisticsFilter.Execute(dataSet);

  vtkm::FloatDefault NValueFromFilter = getStatsFromDataSet(resultDataSet, "N");
  VTKM_TEST_ASSERT(test_equal(NValueFromFilter, N));

  vtkm::FloatDefault MinValueFromFilter = getStatsFromDataSet(resultDataSet, "Min");
  VTKM_TEST_ASSERT(test_equal(MinValueFromFilter, 0));

  vtkm::FloatDefault MaxValueFromFilter = getStatsFromDataSet(resultDataSet, "Max");
  VTKM_TEST_ASSERT(test_equal(MaxValueFromFilter, N - 1));

  vtkm::FloatDefault SumFromFilter = getStatsFromDataSet(resultDataSet, "Sum");
  VTKM_TEST_ASSERT(test_equal(SumFromFilter, N * (N - 1) / 2));

  vtkm::FloatDefault MeanFromFilter = getStatsFromDataSet(resultDataSet, "Mean");
  VTKM_TEST_ASSERT(test_equal(MeanFromFilter, (N - 1) / 2));

  vtkm::FloatDefault SVFromFilter = getStatsFromDataSet(resultDataSet, "SampleVariance");
  VTKM_TEST_ASSERT(test_equal(SVFromFilter, 83416.66));

  vtkm::FloatDefault SstddevFromFilter = getStatsFromDataSet(resultDataSet, "SampleStddev");
  VTKM_TEST_ASSERT(test_equal(SstddevFromFilter, 288.819));

  vtkm::FloatDefault SkewnessFromFilter = getStatsFromDataSet(resultDataSet, "Skewness");
  VTKM_TEST_ASSERT(test_equal(SkewnessFromFilter, 0));

  // we use fisher=False when computing the Kurtosis value
  vtkm::FloatDefault KurtosisFromFilter = getStatsFromDataSet(resultDataSet, "Kurtosis");
  VTKM_TEST_ASSERT(test_equal(KurtosisFromFilter, 1.8));

  vtkm::FloatDefault PopulationStddev = getStatsFromDataSet(resultDataSet, "PopulationStddev");
  VTKM_TEST_ASSERT(test_equal(PopulationStddev, 288.675));

  vtkm::FloatDefault PopulationVariance = getStatsFromDataSet(resultDataSet, "PopulationVariance");
  VTKM_TEST_ASSERT(test_equal(PopulationVariance, 83333.3));
}

void TestStatisticsPartition()
{
  std::vector<vtkm::cont::DataSet> dataSetList;
  constexpr vtkm::FloatDefault N = 1000;

  for (vtkm::Id i = 0; i < 10; i++)
  {
    vtkm::cont::DataSet dataSet;
    constexpr vtkm::FloatDefault localN = N / 10;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray;
    scalarArray.Allocate(static_cast<vtkm::Id>(localN));
    auto writePortal = scalarArray.WritePortal();
    for (vtkm::Id j = 0; j < static_cast<vtkm::Id>(localN); j++)
    {
      writePortal.Set(j, static_cast<vtkm::FloatDefault>(i * localN + j));
    }
    dataSet.AddPointField("scalarField", scalarArray);
    dataSetList.push_back(dataSet);
  }

  //adding data sets for testing edge cases
  vtkm::cont::DataSet dataSetEmptyField;
  dataSetEmptyField.AddPointField("scalarField", vtkm::cont::ArrayHandle<vtkm::FloatDefault>());
  dataSetList.push_back(dataSetEmptyField);

  vtkm::cont::PartitionedDataSet pds(dataSetList);
  using STATS = vtkm::filter::density_estimate::Statistics;
  STATS statisticsFilter;
  using AsscoType = vtkm::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  vtkm::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);

  vtkm::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
  VTKM_TEST_ASSERT(test_equal(NValueFromFilter, N));

  vtkm::FloatDefault MinValueFromFilter = getStatsFromDataSet(outputPDS, "Min");
  VTKM_TEST_ASSERT(test_equal(MinValueFromFilter, 0));

  vtkm::FloatDefault MaxValueFromFilter = getStatsFromDataSet(outputPDS, "Max");
  VTKM_TEST_ASSERT(test_equal(MaxValueFromFilter, N - 1));

  vtkm::FloatDefault SumFromFilter = getStatsFromDataSet(outputPDS, "Sum");
  VTKM_TEST_ASSERT(test_equal(SumFromFilter, N * (N - 1) / 2));

  vtkm::FloatDefault MeanFromFilter = getStatsFromDataSet(outputPDS, "Mean");
  VTKM_TEST_ASSERT(test_equal(MeanFromFilter, (N - 1) / 2));

  vtkm::FloatDefault SVFromFilter = getStatsFromDataSet(outputPDS, "SampleVariance");
  VTKM_TEST_ASSERT(test_equal(SVFromFilter, 83416.66));

  vtkm::FloatDefault SstddevFromFilter = getStatsFromDataSet(outputPDS, "SampleStddev");
  VTKM_TEST_ASSERT(test_equal(SstddevFromFilter, 288.819));

  vtkm::FloatDefault SkewnessFromFilter = getStatsFromDataSet(outputPDS, "Skewness");
  VTKM_TEST_ASSERT(test_equal(SkewnessFromFilter, 0));

  // we use fisher=False when computing the Kurtosis value
  vtkm::FloatDefault KurtosisFromFilter = getStatsFromDataSet(outputPDS, "Kurtosis");
  VTKM_TEST_ASSERT(test_equal(KurtosisFromFilter, 1.8));

  vtkm::FloatDefault PopulationStddev = getStatsFromDataSet(outputPDS, "PopulationStddev");
  VTKM_TEST_ASSERT(test_equal(PopulationStddev, 288.675));

  vtkm::FloatDefault PopulationVariance = getStatsFromDataSet(outputPDS, "PopulationVariance");
  VTKM_TEST_ASSERT(test_equal(PopulationVariance, 83333.3));
}

void TestStatistics()
{
  TestStatisticsPartial();
  TestStatisticsPartition();
} // TestFieldStatistics
}

int UnitTestStatisticsFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStatistics, argc, argv);
}
