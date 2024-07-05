//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRuntimeVec.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/TypeList.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

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

vtkm::cont::UnknownArrayHandle LoadData()
{
  // Read data from some external source where the vector size is determined at runtime.
  std::vector<vtkm::Float32> data;
  int numComponents;
  ReadArray(data, numComponents);

  // Resulting ArrayHandleRuntimeVec gets wrapped in an UnknownArrayHandle
  return vtkm::cont::make_ArrayHandleRuntimeVecMove(
    static_cast<vtkm::IdComponent>(numComponents), std::move(data));
}

void UseVecArray(const vtkm::cont::UnknownArrayHandle& array)
{
  using ExpectedArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f_32>;
  if (!array.CanConvert<ExpectedArrayType>())
  {
    throw vtkm::cont::ErrorBadType("Array unexpected type.");
  }

  ExpectedArrayType concreteArray = array.AsArrayHandle<ExpectedArrayType>();
  // Do something with concreteArray...
  //// PAUSE-EXAMPLE
  VTKM_TEST_ASSERT(concreteArray.GetNumberOfValues() == ARRAY_SIZE);
  //// RESUME-EXAMPLE
}

void LoadAndRun()
{
  // Load data in a routine that does not know component size until runtime.
  vtkm::cont::UnknownArrayHandle array = LoadData();

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

void WriteVTKmArray(const vtkm::cont::UnknownArrayHandle& array)
{
  bool writeSuccess = false;
  auto doWrite = [&](auto componentType) {
    using ComponentType = decltype(componentType);
    using VecArrayType = vtkm::cont::ArrayHandleRuntimeVec<ComponentType>;
    if (array.CanConvert<VecArrayType>())
    {
      // Get the array as a runtime Vec.
      VecArrayType runtimeVecArray = array.AsArrayHandle<VecArrayType>();

      // Get the component array.
      vtkm::cont::ArrayHandleBasic<ComponentType> componentArray =
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
  vtkm::ListForEach(doWrite, vtkm::TypeListBaseC{});
}
////
//// END-EXAMPLE GetRuntimeVec
////

void DoWriteTest()
{
  vtkm::cont::ArrayHandle<vtkm::Vec3f> array;
  array.Allocate(ARRAY_SIZE);
  SetPortal(array.WritePortal());
  WriteVTKmArray(array);
}

void Test()
{
  LoadAndRun();
  DoWriteTest();
}

} // anonymous namespace

int GuideExampleArrayHandleRuntimeVec(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
