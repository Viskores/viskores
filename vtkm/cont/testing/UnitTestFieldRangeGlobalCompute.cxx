//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace
{
static unsigned int uid = 1;

#define PRINT_INFO(msg) std::cout << "[" << comm.rank() << ":" << __LINE__ << "] " msg << std::endl;

#define PRINT_INFO_0(msg)                                                                          \
  if (comm.rank() == 0)                                                                            \
  {                                                                                                \
    std::cout << "[" << comm.rank() << ":" << __LINE__ << "] " msg << std::endl;                   \
  }

template <typename T>
vtkm::cont::ArrayHandle<T> CreateArray(T min, T max, vtkm::Id numVals, vtkm::TypeTraitsScalarTag)
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

template <typename T>
vtkm::cont::ArrayHandle<T> CreateArray(const T& min,
                                       const T& max,
                                       vtkm::Id numVals,
                                       vtkm::TypeTraitsVectorTag)
{
  constexpr int size = T::NUM_COMPONENTS;
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
                  T val;
                  for (int cc = 0; cc < size; ++cc)
                  {
                    val[cc] = static_cast<typename T::ComponentType>(dis[cc](gen));
                  }
                  return val;
                });
  return handle;
}

static constexpr vtkm::Id ARRAY_SIZE = 1025;

template <typename ValueType>
void Validate(const vtkm::cont::ArrayHandle<vtkm::Range>& ranges,
              const ValueType& min,
              const ValueType& max)
{
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  VTKM_TEST_ASSERT(ranges.GetNumberOfValues() == 1, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  auto range = portal.Get(0);
  PRINT_INFO(<< "  expecting [" << min << ", " << max << "], got [" << range.Min << ", "
             << range.Max
             << "]");
  VTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<ValueType>(min) &&
                     range.Max <= static_cast<ValueType>(max),
                   "Got wrong range.");
}

template <typename T, int size>
void Validate(const vtkm::cont::ArrayHandle<vtkm::Range>& ranges,
              const vtkm::Vec<T, size>& min,
              const vtkm::Vec<T, size>& max)
{
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  VTKM_TEST_ASSERT(ranges.GetNumberOfValues() == size, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  for (int cc = 0; cc < size; ++cc)
  {
    auto range = portal.Get(cc);
    PRINT_INFO(<< "  [" << cc << "] expecting [" << min[cc] << ", " << max[cc] << "], got ["
               << range.Min
               << ", "
               << range.Max
               << "]");
    VTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<T>(min[cc]) &&
                       range.Max <= static_cast<T>(max[cc]),
                     "Got wrong range.");
  }
}

template <typename ValueType>
void DecomposeRange(ValueType& min, ValueType& max)
{
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  auto delta = (max - min) / static_cast<ValueType>(comm.size());
  min = min + static_cast<ValueType>(comm.rank()) * delta;
  max = (comm.rank() == comm.size() - 1) ? max : min + delta;
}

template <typename T, int size>
void DecomposeRange(vtkm::Vec<T, size>& min, vtkm::Vec<T, size>& max)
{
  for (int cc = 0; cc < size; ++cc)
  {
    DecomposeRange(min[0], max[0]);
  }
}

template <typename ValueType>
void TryRangeGlobalComputeDS(const ValueType& min, const ValueType& max)
{
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  PRINT_INFO_0("Trying type (dataset): " << vtkm::testing::TypeName<ValueType>::Name());

  // distribute range among all ranks, so we can confirm reduction works.
  ValueType lmin = min, lmax = max;
  DecomposeRange(lmin, lmax);

  PRINT_INFO("gmin=" << min << ", gmax=" << max << " lmin=" << lmin << ", lmax=" << lmax);

  // let's create a dummy dataset with a bunch of fields.
  vtkm::cont::DataSet dataset;
  vtkm::cont::DataSetFieldAdd::AddPointField(
    dataset,
    "pointvar",
    CreateArray(lmin, lmax, ARRAY_SIZE, typename vtkm::TypeTraits<ValueType>::DimensionalityTag()));

  vtkm::cont::ArrayHandle<vtkm::Range> ranges =
    vtkm::cont::FieldRangeGlobalCompute(dataset, "pointvar");
  Validate(ranges, min, max);
}

template <typename ValueType>
void TryRangeGlobalComputePDS(const ValueType& min, const ValueType& max)
{
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  PRINT_INFO("Trying type (PartitionedDataSet): " << vtkm::testing::TypeName<ValueType>::Name());

  vtkm::cont::PartitionedDataSet mb;
  for (int cc = 0; cc < 5; cc++)
  {
    // let's create a dummy dataset with a bunch of fields.
    vtkm::cont::DataSet dataset;
    vtkm::cont::DataSetFieldAdd::AddPointField(
      dataset,
      "pointvar",
      CreateArray(min, max, ARRAY_SIZE, typename vtkm::TypeTraits<ValueType>::DimensionalityTag()));
    mb.AppendPartition(dataset);
  }

  vtkm::cont::ArrayHandle<vtkm::Range> ranges = vtkm::cont::FieldRangeGlobalCompute(mb, "pointvar");
  Validate(ranges, min, max);
}

static void TestFieldRangeGlobalCompute()
{
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  PRINT_INFO_0("Running on " << comm.size() << " ranks.");

  // init random seed.
  std::srand(static_cast<unsigned int>(100 + 1024 * comm.rank()));

  TryRangeGlobalComputeDS<vtkm::Float64>(0, 1000);
  TryRangeGlobalComputeDS<vtkm::Int32>(-1024, 1024);
  TryRangeGlobalComputeDS<vtkm::Vec3f_32>(vtkm::make_Vec(1024, 0, -1024),
                                          vtkm::make_Vec(2048, 2048, 2048));
  TryRangeGlobalComputePDS<vtkm::Float64>(0, 1000);
  TryRangeGlobalComputePDS<vtkm::Int32>(-1024, 1024);
  TryRangeGlobalComputePDS<vtkm::Vec3f_32>(vtkm::make_Vec(1024, 0, -1024),
                                           vtkm::make_Vec(2048, 2048, 2048));
};
}

int UnitTestFieldRangeGlobalCompute(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestFieldRangeGlobalCompute, argc, argv);
}
