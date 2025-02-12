//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/TypeList.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

void ReadArray(std::vector<float>& data, int& numComponents)
{
  numComponents = 3;
  data.resize(static_cast<std::size_t>(ARRAY_SIZE * numComponents));
  std::fill(data.begin(), data.end(), 1.23f);
}

////
//// BEGIN-EXAMPLE GroupWithRuntimeVec
////
void ReadArray(std::vector<float>& data, int& numComponents);

viskores::cont::UnknownArrayHandle LoadData()
{
  // Read data from some external source where the vector size is determined at runtime.
  std::vector<viskores::Float32> data;
  int numComponents;
  ReadArray(data, numComponents);

  // Resulting ArrayHandleRuntimeVec gets wrapped in an UnknownArrayHandle
  return viskores::cont::make_ArrayHandleRuntimeVecMove(
    static_cast<viskores::IdComponent>(numComponents), std::move(data));
}

void UseVecArray(const viskores::cont::UnknownArrayHandle& array)
{
  using ExpectedArrayType = viskores::cont::ArrayHandle<viskores::Vec3f_32>;
  if (!array.CanConvert<ExpectedArrayType>())
  {
    throw viskores::cont::ErrorBadType("Array unexpected type.");
  }

  ExpectedArrayType concreteArray = array.AsArrayHandle<ExpectedArrayType>();
  // Do something with concreteArray...
  //// PAUSE-EXAMPLE
  VISKORES_TEST_ASSERT(concreteArray.GetNumberOfValues() == ARRAY_SIZE);
  //// RESUME-EXAMPLE
}

void LoadAndRun()
{
  // Load data in a routine that does not know component size until runtime.
  viskores::cont::UnknownArrayHandle array = LoadData();

  // Use the data in a method that requires an array of static size.
  // This will work as long as the `Vec` size matches correctly (3 in this case).
  UseVecArray(array);
}
////
//// END-EXAMPLE GroupWithRuntimeVec
////

template<typename T>
void WriteData(const T*, std::size_t, int)
{
  // Dummy function for GetRuntimeVec.
}

////
//// BEGIN-EXAMPLE GetRuntimeVec
////
template<typename T>
void WriteData(const T* data, std::size_t size, int numComponents);

void WriteViskoresArray(const viskores::cont::UnknownArrayHandle& array)
{
  bool writeSuccess = false;
  auto doWrite = [&](auto componentType) {
    using ComponentType = decltype(componentType);
    using VecArrayType = viskores::cont::ArrayHandleRuntimeVec<ComponentType>;
    if (array.CanConvert<VecArrayType>())
    {
      // Get the array as a runtime Vec.
      VecArrayType runtimeVecArray = array.AsArrayHandle<VecArrayType>();

      // Get the component array.
      viskores::cont::ArrayHandleBasic<ComponentType> componentArray =
        runtimeVecArray.GetComponentsArray();

      // Use the general function to write the data.
      WriteData(componentArray.GetReadPointer(),
                componentArray.GetNumberOfValues(),
                runtimeVecArray.GetNumberOfComponentsFlat());

      writeSuccess = true;
    }
  };

  // Figure out the base component type, retrieve the data (regardless
  // of vec size), and write out the data.
  viskores::ListForEach(doWrite, viskores::TypeListBaseC{});
}
////
//// END-EXAMPLE GetRuntimeVec
////

void DoWriteTest()
{
  viskores::cont::ArrayHandle<viskores::Vec3f> array;
  array.Allocate(ARRAY_SIZE);
  SetPortal(array.WritePortal());
  WriteViskoresArray(array);
}

void Test()
{
  LoadAndRun();
  DoWriteTest();
}

} // anonymous namespace

int GuideExampleArrayHandleRuntimeVec(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
