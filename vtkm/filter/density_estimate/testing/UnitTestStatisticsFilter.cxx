//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/density_estimate/Statistics.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>


namespace
{

vtkm::FloatDefault getStatsFromArray(vtkm::cont::DataSet dataset, const std::string statName)
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
  // the number is from 0 to 999
  auto scalaArray =
    vtkm::cont::ArrayHandleCounting<vtkm::FloatDefault>(0.0f, 1.0f, static_cast<vtkm::Id>(N));
  dataSet.AddPointField("scalarField", scalaArray);

  using STATS = vtkm::filter::density_estimate::Statistics;
  STATS statisticsFilter;

  //set required states
  std::vector<STATS::Stats> RequiredStatsList{ STATS::Stats::N,        STATS::Stats::Sum,
                                               STATS::Stats::Mean,     STATS::Stats::SampleVariance,
                                               STATS::Stats::Skewness, STATS::Stats::Kurtosis };

  //default RequiredStatsList contains all statistics variables
  statisticsFilter.SetRequiredStats(RequiredStatsList);
  using AsscoType = vtkm::cont::Field::Association;

  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);

  // We use the same test cases with the UnitTestDescriptiveStatistics.h
  vtkm::cont::DataSet resultDataSet = statisticsFilter.Execute(dataSet);

  vtkm::FloatDefault NValueFromFilter = getStatsFromArray(resultDataSet, "N");
  VTKM_TEST_ASSERT(test_equal(NValueFromFilter, N));

  vtkm::FloatDefault SumFromFilter = getStatsFromArray(resultDataSet, "Sum");
  VTKM_TEST_ASSERT(test_equal(SumFromFilter, N * (N - 1) / 2));

  vtkm::FloatDefault MeanFromFilter = getStatsFromArray(resultDataSet, "Mean");
  VTKM_TEST_ASSERT(test_equal(MeanFromFilter, (N - 1) / 2));

  vtkm::FloatDefault SVFromFilter = getStatsFromArray(resultDataSet, "SampleVariance");
  VTKM_TEST_ASSERT(test_equal(SVFromFilter, 83416.66));

  vtkm::FloatDefault SkewnessFromFilter = getStatsFromArray(resultDataSet, "Skewness");
  VTKM_TEST_ASSERT(test_equal(SkewnessFromFilter, 0));

  // we use fisher=False when computing the Kurtosis value
  vtkm::FloatDefault KurtosisFromFilter = getStatsFromArray(resultDataSet, "Kurtosis");
  VTKM_TEST_ASSERT(test_equal(KurtosisFromFilter, 1.8));
}


void TestStatistics()
{
  TestStatisticsPartial();
} // TestFieldStatistics
}

//More deatiled tests can be found in the UnitTestStatisticsFilter
int UnitTestStatisticsFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStatistics, argc, argv);
}
