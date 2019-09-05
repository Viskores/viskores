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
#include <vtkm/cont/ArrayHandleIndex.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename T>
VTKM_CONT void TestValues(const vtkm::cont::ArrayHandle<T>& ah,
                          const std::initializer_list<T>& expected)
{
  auto portal = ah.GetPortalConstControl();
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
    const std::vector<vtkm::Id> idsVec{ 3, 8, 7 };
    const auto ids = vtkm::cont::make_ArrayHandle(idsVec);
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

void Test()
{
  TryCopy<vtkm::Id>();
  TryCopy<vtkm::IdComponent>();
  TryCopy<vtkm::Float32>();
}

} // anonymous namespace

int UnitTestArrayGetValues(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
