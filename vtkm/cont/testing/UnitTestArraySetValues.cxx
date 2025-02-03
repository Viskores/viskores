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
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArraySetValues.h>

#include <vtkm/Bounds.h>
#include <vtkm/Range.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename T>
VTKM_CONT void TestValues(const vtkm::cont::ArrayHandle<T>& ah,
                          const std::initializer_list<T>& expected)
{
  auto portal = ah.ReadPortal();
  VTKM_TEST_ASSERT(expected.size() == static_cast<size_t>(ah.GetNumberOfValues()));
  for (vtkm::Id i = 0; i < ah.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(expected.begin()[static_cast<size_t>(i)] == portal.Get(i));
  }
}

template <typename ValueType>
void TryCopy()
{
  std::cout << "Trying type: " << vtkm::testing::TypeName<ValueType>::Name() << std::endl;

  auto createData = []() -> vtkm::cont::ArrayHandle<ValueType> {
    vtkm::cont::ArrayHandle<ValueType> data;
    // Create and initialize the ValueType array
    vtkm::cont::ArrayHandleIndex values(ARRAY_SIZE);
    vtkm::cont::ArrayCopy(values, data);
    return data;
  };

  { // ArrayHandle ids
    const auto ids = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 3, 8, 7 });
    { // Pass vector
      const auto data = createData();
      std::vector<ValueType> values{ 30, 80, 70 };
      vtkm::cont::ArraySetValues(ids, values, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
    { // Pass Handle
      const auto data = createData();
      const auto newValues = vtkm::cont::make_ArrayHandle<ValueType>({ 30, 80, 70 });
      vtkm::cont::ArraySetValues(ids, newValues, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
    { // Test the specialization for ArrayHandleCast
      const auto data = createData();
      auto castedData = vtkm::cont::make_ArrayHandleCast<vtkm::Float64>(data);
      const auto doubleValues = vtkm::cont::make_ArrayHandle<vtkm::Float64>({ 3.0, 8.0, 7.0 });
      vtkm::cont::ArraySetValues(ids, doubleValues, castedData);
      TestValues<ValueType>(data, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    }
  }

  { // vector ids
    const std::vector<vtkm::Id> ids{ 3, 8, 7 };
    { // Pass vector
      const auto data = createData();
      const std::vector<ValueType> values{ 30, 80, 70 };
      vtkm::cont::ArraySetValues(ids, values, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
    { // Pass handle
      const auto data = createData();
      const auto newValues = vtkm::cont::make_ArrayHandle<ValueType>({ 30, 80, 70 });
      vtkm::cont::ArraySetValues(ids, newValues, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
  }

  {   // Initializer list ids
    { // Pass vector:
      const auto data = createData();
  const std::vector<ValueType> values{ 30, 80, 70 };
  vtkm::cont::ArraySetValues({ 3, 8, 7 }, values, data);
  TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
}
{ // Pass initializer list
  const auto data = createData();
  vtkm::cont::ArraySetValues(
    { 3, 8, 7 },
    { static_cast<ValueType>(30), static_cast<ValueType>(80), static_cast<ValueType>(70) },
    data);
  TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
}
{ // Pass handle:
  const auto data = createData();
  const auto newValues = vtkm::cont::make_ArrayHandle<ValueType>({ 30, 80, 70 });
  vtkm::cont::ArraySetValues({ 3, 8, 7 }, newValues, data);
  TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
}
}

{ // c-array ids
  const std::vector<vtkm::Id> idVec{ 3, 8, 7 };
  const vtkm::Id* ids = idVec.data();
  const auto numIds = static_cast<vtkm::Id>(idVec.size());
  const std::vector<ValueType> valueVec{ 30, 80, 70 };
  const ValueType* values = valueVec.data();
  const auto nValues = static_cast<vtkm::Id>(valueVec.size());
  { // Pass c-array
    const auto data = createData();
    vtkm::cont::ArraySetValues(ids, numIds, values, nValues, data);
    TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
  }
  { // Pass vector
    const auto data = createData();
    vtkm::cont::ArraySetValues(ids, numIds, valueVec, data);
    TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
  }
  { // Pass Handle
    const auto data = createData();
    const auto newValues = vtkm::cont::make_ArrayHandle<ValueType>(valueVec, vtkm::CopyFlag::Off);
    vtkm::cont::ArraySetValues(ids, numIds, newValues, data);
    TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
  }
}

{ // single value
  const auto data = createData();
  vtkm::cont::ArraySetValue(8, static_cast<ValueType>(88), data);
  TestValues<ValueType>(data, { 0, 1, 2, 3, 4, 5, 6, 7, 88, 9 });
}
}

void TryRange()
{
  std::cout << "Trying vtkm::Range" << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Range> values =
    vtkm::cont::make_ArrayHandle<vtkm::Range>({ { 0.0, 1.0 }, { 1.0, 2.0 }, { 2.0, 4.0 } });

  vtkm::cont::ArraySetValue(1, vtkm::Range{ 5.0, 6.0 }, values);
  auto portal = values.ReadPortal();
  VTKM_TEST_ASSERT(portal.Get(1) == vtkm::Range{ 5.0, 6.0 });
}

void TryBounds()
{
  std::cout << "Trying vtkm::Bounds" << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Bounds> values =
    vtkm::cont::make_ArrayHandle<vtkm::Bounds>({ { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } },
                                                 { { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 } },
                                                 { { 2.0, 4.0 }, { 2.0, 4.0 }, { 2.0, 4.0 } } });

  vtkm::cont::ArraySetValue(1, vtkm::Bounds{ { 5.0, 6.0 }, { 5.0, 6.0 }, { 5.0, 6.0 } }, values);
  auto portal = values.ReadPortal();
  VTKM_TEST_ASSERT(portal.Get(1) == vtkm::Bounds{ { 5.0, 6.0 }, { 5.0, 6.0 }, { 5.0, 6.0 } });
}

void Test()
{
  TryCopy<vtkm::Id>();
  TryCopy<vtkm::IdComponent>();
  TryCopy<vtkm::Float32>();
  TryRange();
  TryBounds();
}

} // anonymous namespace

int UnitTestArraySetValues(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
