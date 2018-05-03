//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

static unsigned int uid = 1;

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
  VTKM_TEST_ASSERT(ranges.GetNumberOfValues() == 1, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  auto range = portal.Get(0);
  std::cout << "  expecting [" << min << ", " << max << "], got [" << range.Min << ", " << range.Max
            << "]" << std::endl;
  VTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<ValueType>(min) &&
                     range.Max <= static_cast<ValueType>(max),
                   "Got wrong range.");
}

template <typename T, int size>
void Validate(const vtkm::cont::ArrayHandle<vtkm::Range>& ranges,
              const vtkm::Vec<T, size>& min,
              const vtkm::Vec<T, size>& max)
{
  VTKM_TEST_ASSERT(ranges.GetNumberOfValues() == size, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  for (int cc = 0; cc < size; ++cc)
  {
    auto range = portal.Get(cc);
    std::cout << "  [0] expecting [" << min[cc] << ", " << max[cc] << "], got [" << range.Min
              << ", " << range.Max << "]" << std::endl;
    VTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<T>(min[cc]) &&
                       range.Max <= static_cast<T>(max[cc]),
                     "Got wrong range.");
  }
}

template <typename ValueType>
void TryRangeComputeDS(const ValueType& min, const ValueType& max)
{
  std::cout << "Trying type (dataset): " << vtkm::testing::TypeName<ValueType>::Name() << std::endl;
  // let's create a dummy dataset with a bunch of fields.
  vtkm::cont::DataSet dataset;
  vtkm::cont::DataSetFieldAdd::AddPointField(
    dataset,
    "pointvar",
    CreateArray(min, max, ARRAY_SIZE, typename vtkm::TypeTraits<ValueType>::DimensionalityTag()));

  vtkm::cont::ArrayHandle<vtkm::Range> ranges = vtkm::cont::FieldRangeCompute(dataset, "pointvar");
  Validate(ranges, min, max);
}

template <typename ValueType>
void TryRangeComputeMB(const ValueType& min, const ValueType& max)
{
  std::cout << "Trying type (multiblock): " << vtkm::testing::TypeName<ValueType>::Name()
            << std::endl;

  vtkm::cont::MultiBlock mb;
  for (int cc = 0; cc < 5; cc++)
  {
    // let's create a dummy dataset with a bunch of fields.
    vtkm::cont::DataSet dataset;
    vtkm::cont::DataSetFieldAdd::AddPointField(
      dataset,
      "pointvar",
      CreateArray(min, max, ARRAY_SIZE, typename vtkm::TypeTraits<ValueType>::DimensionalityTag()));
    mb.AddBlock(dataset);
  }

  vtkm::cont::ArrayHandle<vtkm::Range> ranges = vtkm::cont::FieldRangeCompute(mb, "pointvar");
  Validate(ranges, min, max);
}

static void TestFieldRangeCompute()
{
  // init random seed.
  std::srand(100);

  TryRangeComputeDS<vtkm::Float64>(0, 1000);
  TryRangeComputeDS<vtkm::Int32>(-1024, 1024);
  TryRangeComputeDS<vtkm::Vec<vtkm::Float32, 3>>(vtkm::make_Vec(1024, 0, -1024),
                                                 vtkm::make_Vec(2048, 2048, 2048));
  TryRangeComputeMB<vtkm::Float64>(0, 1000);
  TryRangeComputeMB<vtkm::Int32>(-1024, 1024);
  TryRangeComputeMB<vtkm::Vec<vtkm::Float32, 3>>(vtkm::make_Vec(1024, 0, -1024),
                                                 vtkm::make_Vec(2048, 2048, 2048));
};

int UnitTestFieldRangeCompute(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestFieldRangeCompute);
}
