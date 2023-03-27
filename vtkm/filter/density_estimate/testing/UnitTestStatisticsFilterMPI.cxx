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
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/density_estimate/Statistics.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/environment.h>

namespace
{
vtkm::FloatDefault getStatsFromDataSet(const vtkm::cont::PartitionedDataSet& dataset,
                                       const std::string statName)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array;
  dataset.GetField(statName).GetData().AsArrayHandle(array);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType portal = array.ReadPortal();
  vtkm::FloatDefault value = portal.Get(0);
  return value;
}

void checkResulst(const vtkm::cont::PartitionedDataSet& outputPDS, vtkm::FloatDefault N)
{
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

void TestStatisticsMPISingleDataSet()
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  constexpr vtkm::FloatDefault N = 1000;
  vtkm::Id numProcs = comm.size();

  vtkm::Id workloadBase = static_cast<vtkm::Id>(N / numProcs);
  vtkm::Id workloadActual = workloadBase;
  if (static_cast<vtkm::Id>(N) % numProcs != 0)
  {
    //updating the workload for last one
    if (comm.rank() == numProcs - 1)
    {
      workloadActual = workloadActual + (static_cast<vtkm::Id>(N) % numProcs);
    }
  }

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray;
  scalarArray.Allocate(static_cast<vtkm::Id>(workloadActual));
  auto writePortal = scalarArray.WritePortal();
  for (vtkm::Id i = 0; i < static_cast<vtkm::Id>(workloadActual); i++)
  {
    writePortal.Set(i, static_cast<vtkm::FloatDefault>(workloadBase * comm.rank() + i));
  }

  vtkm::cont::DataSet dataSet;
  dataSet.AddPointField("scalarField", scalarArray);

  using STATS = vtkm::filter::density_estimate::Statistics;
  STATS statisticsFilter;

  using AsscoType = vtkm::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  std::vector<vtkm::cont::DataSet> dataSetList;
  dataSetList.push_back(dataSet);
  auto pds = vtkm::cont::PartitionedDataSet(dataSetList);
  vtkm::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);

  if (comm.rank() == 0)
  {
    checkResulst(outputPDS, N);
  }
  else
  {
    vtkm::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VTKM_TEST_ASSERT(test_equal(NValueFromFilter, 0));
  }
}

void TestStatisticsMPIPartitionDataSets()
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  constexpr vtkm::FloatDefault N = 1000;
  vtkm::Id numProcs = comm.size();

  vtkm::Id workloadPerRankBase = static_cast<vtkm::Id>(N / numProcs);
  vtkm::Id workloadPerRankActual = workloadPerRankBase;

  if (static_cast<vtkm::Id>(N) % numProcs != 0)
  {
    //updating the workload for last one
    if (comm.rank() == numProcs - 1)
    {
      workloadPerRankActual = workloadPerRankActual + (static_cast<vtkm::Id>(N) % numProcs);
    }
  }

  vtkm::Id numPartitions = 2;
  vtkm::Id workloadPerPartition0 = workloadPerRankActual / numPartitions;
  vtkm::Id workloadPerPartition1 = workloadPerRankActual - workloadPerPartition0;

  vtkm::Id offsetRank = workloadPerRankBase * comm.rank();
  std::vector<vtkm::cont::DataSet> dataSetList;

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray0;
  scalarArray0.Allocate(static_cast<vtkm::Id>(workloadPerPartition0));
  auto writePortal0 = scalarArray0.WritePortal();
  vtkm::cont::DataSet dataSet0;

  for (vtkm::Id i = 0; i < workloadPerPartition0; i++)
  {
    writePortal0.Set(i, static_cast<vtkm::FloatDefault>(offsetRank + i));
  }

  dataSet0.AddPointField("scalarField", scalarArray0);
  dataSetList.push_back(dataSet0);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray1;
  scalarArray1.Allocate(static_cast<vtkm::Id>(workloadPerPartition1));
  auto writePortal1 = scalarArray1.WritePortal();
  vtkm::cont::DataSet dataSet1;

  for (vtkm::Id i = 0; i < workloadPerPartition1; i++)
  {
    writePortal1.Set(i, static_cast<vtkm::FloatDefault>(offsetRank + workloadPerPartition0 + i));
  }

  dataSet1.AddPointField("scalarField", scalarArray1);
  dataSetList.push_back(dataSet1);

  auto pds = vtkm::cont::PartitionedDataSet(dataSetList);

  using STATS = vtkm::filter::density_estimate::Statistics;
  STATS statisticsFilter;
  using AsscoType = vtkm::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);

  vtkm::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);
  if (comm.rank() == 0)
  {
    checkResulst(outputPDS, N);
  }
  else
  {
    vtkm::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VTKM_TEST_ASSERT(test_equal(NValueFromFilter, 0));
  }
}

void TestStatisticsMPIDataSetEmpty()
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  constexpr vtkm::FloatDefault N = 1000;
  vtkm::Id numProcs = comm.size();
  vtkm::Id numEmptyBlock = 1;
  vtkm::Id numProcsWithWork = numProcs;
  if (numProcs > 1)
  {
    numProcsWithWork = numProcsWithWork - numEmptyBlock;
  }

  vtkm::Id workloadBase = static_cast<vtkm::Id>(N / (numProcsWithWork));
  vtkm::Id workloadActual = workloadBase;
  if (static_cast<vtkm::Id>(N) % numProcsWithWork != 0)
  {
    //updating the workload for last one
    if (comm.rank() == numProcsWithWork - 1)
    {
      workloadActual = workloadActual + (static_cast<vtkm::Id>(N) % numProcsWithWork);
    }
  }

  vtkm::cont::DataSet dataSet;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray;
  //for the proc with actual work
  if (comm.rank() != numProcs - 1)
  {
    scalarArray.Allocate(static_cast<vtkm::Id>(workloadActual));
    auto writePortal = scalarArray.WritePortal();
    for (vtkm::Id i = 0; i < static_cast<vtkm::Id>(workloadActual); i++)
    {
      writePortal.Set(i, static_cast<vtkm::FloatDefault>(workloadBase * comm.rank() + i));
    }
  }
  dataSet.AddPointField("scalarField", scalarArray);

  using STATS = vtkm::filter::density_estimate::Statistics;
  STATS statisticsFilter;

  using AsscoType = vtkm::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  std::vector<vtkm::cont::DataSet> dataSetList;
  dataSetList.push_back(dataSet);
  auto pds = vtkm::cont::PartitionedDataSet(dataSetList);
  vtkm::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);

  if (comm.size() == 1)
  {
    vtkm::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VTKM_TEST_ASSERT(test_equal(NValueFromFilter, 0));
    return;
  }

  if (comm.rank() == 0)
  {
    checkResulst(outputPDS, N);
  }
  else
  {
    vtkm::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VTKM_TEST_ASSERT(test_equal(NValueFromFilter, 0));
  }
}

void TestStatistics()
{
  TestStatisticsMPISingleDataSet();
  TestStatisticsMPIPartitionDataSets();
  TestStatisticsMPIDataSetEmpty();
} // TestFieldStatistics
}

//More deatiled tests can be found in the UnitTestStatisticsFilter
int UnitTestStatisticsFilterMPI(int argc, char* argv[])
{
  vtkmdiy::mpi::environment env(argc, argv);
  vtkmdiy::mpi::communicator world;
  return vtkm::cont::testing::Testing::Run(TestStatistics, argc, argv);
}
