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
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleIndex.h>

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

template <typename T>
VTKM_CONT void TestValues(const std::vector<T>& vec, const std::initializer_list<T>& expected)
{
  VTKM_TEST_ASSERT(expected.size() == vec.size());
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    VTKM_TEST_ASSERT(expected.begin()[static_cast<size_t>(i)] == vec[i]);
  }
}

template <typename ValueType>
void TryCopy()
{
  std::cout << "Trying type: " << vtkm::testing::TypeName<ValueType>::Name() << std::endl;

  vtkm::cont::ArrayHandle<ValueType> data;
  { // Create the ValueType array.
    vtkm::cont::ArrayHandleIndex values(ARRAY_SIZE);
    vtkm::cont::ArrayCopy(values, data);
  }

  { // ArrayHandle ids
    const auto ids = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 3, 8, 7 });
    { // Return vector:
      const std::vector<ValueType> output = vtkm::cont::ArrayGetValues(ids, data);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Pass vector:
      std::vector<ValueType> output;
      vtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Pass handle:
      vtkm::cont::ArrayHandle<ValueType> output;
      vtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Test the specialization for ArrayHandleCast
      auto castedData = vtkm::cont::make_ArrayHandleCast<vtkm::Float64>(data);
      vtkm::cont::ArrayHandle<vtkm::Float64> output;
      vtkm::cont::ArrayGetValues(ids, castedData, output);
      TestValues<vtkm::Float64>(output, { 3.0, 8.0, 7.0 });
    }
  }

  { // vector ids
    const std::vector<vtkm::Id> ids{ 1, 5, 3, 9 };
    { // Return vector:
      const std::vector<ValueType> output = vtkm::cont::ArrayGetValues(ids, data);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
    { // Pass vector:
      std::vector<ValueType> output;
      vtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
    { // Pass handle:
      vtkm::cont::ArrayHandle<ValueType> output;
      vtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
  }

  {   // Initializer list ids
    { // Return vector:
      const std::vector<ValueType> output = vtkm::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
{ // Pass vector:
  std::vector<ValueType> output;
  vtkm::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data, output);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
{ // Pass handle:
  vtkm::cont::ArrayHandle<ValueType> output;
  vtkm::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data, output);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
}

{ // c-array ids
  const std::vector<vtkm::Id> idVec{ 8, 6, 7, 5, 3, 0, 9 };
  const vtkm::Id* ids = idVec.data();
  const vtkm::Id n = static_cast<vtkm::Id>(idVec.size());
  { // Return vector:
    const std::vector<ValueType> output = vtkm::cont::ArrayGetValues(ids, n, data);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
  { // Pass vector:
    std::vector<ValueType> output;
    vtkm::cont::ArrayGetValues(ids, n, data, output);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
  { // Pass handle:
    vtkm::cont::ArrayHandle<ValueType> output;
    vtkm::cont::ArrayGetValues(ids, n, data, output);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
}

{ // single values
  {
    const ValueType output = vtkm::cont::ArrayGetValue(8, data);
    VTKM_TEST_ASSERT(output == static_cast<ValueType>(8));
  }
  {
    ValueType output;
    vtkm::cont::ArrayGetValue(8, data, output);
    VTKM_TEST_ASSERT(output == static_cast<ValueType>(8));
  }
}
}

void TryRange()
{
  std::cout << "Trying vtkm::Range" << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Range> values =
    vtkm::cont::make_ArrayHandle<vtkm::Range>({ { 0.0, 1.0 }, { 1.0, 2.0 }, { 2.0, 4.0 } });
  vtkm::Range range = vtkm::cont::ArrayGetValue(1, values);
  VTKM_TEST_ASSERT(range == vtkm::Range{ 1.0, 2.0 });
}

void TryBounds()
{
  std::cout << "Trying vtkm::Bounds" << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Bounds> values =
    vtkm::cont::make_ArrayHandle<vtkm::Bounds>({ { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } },
                                                 { { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 } },
                                                 { { 2.0, 4.0 }, { 2.0, 4.0 }, { 2.0, 4.0 } } });
  vtkm::Bounds bounds = vtkm::cont::ArrayGetValue(1, values);
  VTKM_TEST_ASSERT(bounds == vtkm::Bounds{ { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 } });
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

int UnitTestArrayGetValues(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
