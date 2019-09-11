//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/Histogram.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

namespace
{

static unsigned int uid = 1;

template <typename T>
vtkm::cont::ArrayHandle<T> CreateArrayHandle(T min, T max, vtkm::Id numVals)
{
  std::mt19937 gen(uid++);
  std::uniform_real_distribution<double> dis(static_cast<double>(min), static_cast<double>(max));

  vtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);

  std::generate(vtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()),
                vtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalControl()),
                [&]() { return static_cast<T>(dis(gen)); });
  return handle;
}

template <typename T, int size>
vtkm::cont::ArrayHandle<vtkm::Vec<T, size>> CreateArrayHandle(const vtkm::Vec<T, size>& min,
                                                              const vtkm::Vec<T, size>& max,
                                                              vtkm::Id numVals)
{
  std::mt19937 gen(uid++);
  std::uniform_real_distribution<double> dis[size];
  for (int cc = 0; cc < size; ++cc)
  {
    dis[cc] = std::uniform_real_distribution<double>(static_cast<double>(min[cc]),
                                                     static_cast<double>(max[cc]));
  }
  vtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);
  std::generate(vtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()),
                vtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalControl()),
                [&]() {
                  vtkm::Vec<T, size> val;
                  for (int cc = 0; cc < size; ++cc)
                  {
                    val[cc] = static_cast<T>(dis[cc](gen));
                  }
                  return val;
                });
  return handle;
}


template <typename T>
void AddField(vtkm::cont::DataSet& dataset,
              const T& min,
              const T& max,
              vtkm::Id numVals,
              const std::string& name,
              vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::POINTS)
{
  auto ah = CreateArrayHandle(min, max, numVals);
  dataset.AddField(vtkm::cont::Field(name, assoc, ah));
}
}

static void TestPartitionedDataSetHistogram()
{
  // init random seed.
  std::srand(100);

  vtkm::cont::PartitionedDataSet mb;

  vtkm::cont::DataSet partition0;
  AddField<double>(partition0, 0.0, 100.0, 1024, "double");
  mb.AppendPartition(partition0);

  vtkm::cont::DataSet partition1;
  AddField<int>(partition1, 100, 1000, 1024, "double");
  mb.AppendPartition(partition1);

  vtkm::cont::DataSet partition2;
  AddField<double>(partition2, 100.0, 500.0, 1024, "double");
  mb.AppendPartition(partition2);

  vtkm::filter::Histogram histogram;
  histogram.SetActiveField("double");
  auto result = histogram.Execute(mb);
  VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == 1, "Expecting 1 partition.");

  auto bins = result.GetPartition(0)
                .GetField("histogram")
                .GetData()
                .Cast<vtkm::cont::ArrayHandle<vtkm::Id>>();
  VTKM_TEST_ASSERT(bins.GetNumberOfValues() == 10, "Expecting 10 bins.");
  auto count = std::accumulate(vtkm::cont::ArrayPortalToIteratorBegin(bins.GetPortalConstControl()),
                               vtkm::cont::ArrayPortalToIteratorEnd(bins.GetPortalConstControl()),
                               vtkm::Id(0),
                               vtkm::Add());
  VTKM_TEST_ASSERT(count == 1024 * 3, "Expecting 3072 values");

  std::cout << "Values [" << count << "] =";
  for (int cc = 0; cc < 10; ++cc)
  {
    std::cout << " " << bins.GetPortalConstControl().Get(cc);
  }
  std::cout << std::endl;
};

int UnitTestPartitionedDataSetHistogramFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPartitionedDataSetHistogram, argc, argv);
}
