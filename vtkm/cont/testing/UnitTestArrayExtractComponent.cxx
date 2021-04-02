//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayExtractComponent.h>

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleView.h>

#include <vtkm/VecFlat.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename T, typename S>
void CheckInputArray(const vtkm::cont::ArrayHandle<T, S>& originalArray,
                     vtkm::CopyFlag allowCopy = vtkm::CopyFlag::Off)
{
  //std::cout << "  Checking input array type "
  //          << vtkm::cont::TypeToString<vtkm::cont::ArrayHandle<T, S>>() << std::endl;

  //std::cout << "    Original array: ";
  //vtkm::cont::printSummary_ArrayHandle(originalArray, std::cout);

  using FlatVec = vtkm::VecFlat<T>;
  using ComponentType = typename FlatVec::ComponentType;
  for (vtkm::IdComponent componentId = 0; componentId < FlatVec::NUM_COMPONENTS; ++componentId)
  {
    vtkm::cont::ArrayHandleStride<ComponentType> componentArray =
      vtkm::cont::ArrayExtractComponent(originalArray, componentId, allowCopy);
    //std::cout << "    Component " << componentId << ": ";
    //vtkm::cont::printSummary_ArrayHandle(componentArray, std::cout);

    auto originalPortal = originalArray.ReadPortal();
    auto componentPortal = componentArray.ReadPortal();
    VTKM_TEST_ASSERT(originalPortal.GetNumberOfValues() == componentPortal.GetNumberOfValues());
    for (vtkm::Id arrayIndex = 0; arrayIndex < originalArray.GetNumberOfValues(); ++arrayIndex)
    {
      auto originalValue = vtkm::make_VecFlat(originalPortal.Get(arrayIndex));
      ComponentType componentValue = componentPortal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(originalValue[componentId], componentValue));
    }
  }
}

template <typename T, typename S>
void CheckOutputArray(const vtkm::cont::ArrayHandle<T, S>& originalArray)
{
  CheckInputArray(originalArray);

  //std::cout << "  Checking output array type "
  //          << vtkm::cont::TypeToString<vtkm::cont::ArrayHandle<T, S>>() << std::endl;

  //std::cout << "    Original array: ";
  //vtkm::cont::printSummary_ArrayHandle(originalArray, std::cout);

  vtkm::cont::ArrayHandle<T, S> outputArray;
  outputArray.Allocate(originalArray.GetNumberOfValues());

  using FlatVec = vtkm::VecFlat<T>;
  using ComponentType = typename FlatVec::ComponentType;
  constexpr vtkm::IdComponent numComponents = FlatVec::NUM_COMPONENTS;
  for (vtkm::IdComponent componentId = 0; componentId < numComponents; ++componentId)
  {
    vtkm::cont::ArrayHandleStride<ComponentType> inComponentArray =
      vtkm::cont::ArrayExtractComponent(originalArray, numComponents - componentId - 1);
    vtkm::cont::ArrayHandleStride<ComponentType> outComponentArray =
      vtkm::cont::ArrayExtractComponent(outputArray, componentId, vtkm::CopyFlag::Off);

    auto inPortal = inComponentArray.ReadPortal();
    auto outPortal = outComponentArray.WritePortal();
    VTKM_TEST_ASSERT(inComponentArray.GetNumberOfValues() == originalArray.GetNumberOfValues());
    VTKM_TEST_ASSERT(outComponentArray.GetNumberOfValues() == originalArray.GetNumberOfValues());
    for (vtkm::Id arrayIndex = 0; arrayIndex < originalArray.GetNumberOfValues(); ++arrayIndex)
    {
      outPortal.Set(arrayIndex, inPortal.Get(arrayIndex));
    }
  }

  //std::cout << "    Output array: ";
  //vtkm::cont::printSummary_ArrayHandle(outputArray, std::cout);

  auto inPortal = originalArray.ReadPortal();
  auto outPortal = outputArray.ReadPortal();
  for (vtkm::Id arrayIndex = 0; arrayIndex < originalArray.GetNumberOfValues(); ++arrayIndex)
  {
    FlatVec inValue = vtkm::make_VecFlat(inPortal.Get(arrayIndex));
    FlatVec outValue = vtkm::make_VecFlat(outPortal.Get(arrayIndex));
    for (vtkm::IdComponent componentId = 0; componentId < numComponents; ++componentId)
    {
      VTKM_TEST_ASSERT(test_equal(inValue[componentId], outValue[numComponents - componentId - 1]));
    }
  }
}

void DoTest()
{
  using ArrayMultiplexerType =
    vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandleBasic<vtkm::Vec3f>,
                                       vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>;

  {
    std::cout << "Basic array" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckOutputArray(array);

    std::cout << "ArrayHandleExtractComponent" << std::endl;
    CheckOutputArray(vtkm::cont::make_ArrayHandleExtractComponent(array, 1));

    std::cout << "ArrayHandleMultiplexer" << std::endl;
    CheckInputArray(ArrayMultiplexerType(array));
  }

  {
    std::cout << "SOA array" << std::endl;
    vtkm::cont::ArrayHandleSOA<vtkm::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckOutputArray(array);

    CheckInputArray(ArrayMultiplexerType(array));
  }

  {
    std::cout << "Stride array" << std::endl;
    constexpr vtkm::Id STRIDE = 7;
    vtkm::cont::ArrayHandleBasic<vtkm::Vec3f> originalArray;
    originalArray.Allocate(ARRAY_SIZE * STRIDE);
    for (vtkm::Id offset = 0; offset < STRIDE; ++offset)
    {
      vtkm::cont::ArrayHandleStride<vtkm::Vec3f> strideArray(
        originalArray, ARRAY_SIZE, STRIDE, offset);
      CheckInputArray(strideArray);
    }
  }

  {
    std::cout << "ArrayHandleGroupVec" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> array;
    array.Allocate(ARRAY_SIZE * 2);
    SetPortal(array.WritePortal());
    CheckOutputArray(vtkm::cont::make_ArrayHandleGroupVec<2>(array));
  }

  {
    std::cout << "ArrayHandleCompositeVector" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> array0;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> array1;
    array0.Allocate(ARRAY_SIZE);
    array1.Allocate(ARRAY_SIZE);
    SetPortal(array0.WritePortal());
    SetPortal(array1.WritePortal());
    auto compositeArray = vtkm::cont::make_ArrayHandleCompositeVector(array0, array1);
    CheckOutputArray(compositeArray);

    CheckOutputArray(vtkm::cont::make_ArrayHandleExtractComponent(compositeArray, 1));
  }

  {
    std::cout << "ArrayHandleCartesianProduct" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Float64> array0;
    vtkm::cont::ArrayHandle<vtkm::Float64> array1;
    vtkm::cont::ArrayHandle<vtkm::Float64> array2;
    array0.Allocate(ARRAY_SIZE);
    array1.Allocate(ARRAY_SIZE / 2);
    array2.Allocate(ARRAY_SIZE + 2);
    SetPortal(array0.WritePortal());
    SetPortal(array1.WritePortal());
    SetPortal(array2.WritePortal());
    CheckInputArray(vtkm::cont::make_ArrayHandleCartesianProduct(array0, array1, array2));
  }

  {
    std::cout << "ArrayHandleUniformPointCoordinates" << std::endl;
    vtkm::cont::ArrayHandleUniformPointCoordinates array(
      vtkm::Id3{ ARRAY_SIZE, ARRAY_SIZE + 2, ARRAY_SIZE / 2 });
    CheckInputArray(array, vtkm::CopyFlag::On);
  }

  {
    std::cout << "ArrayHandleReverse" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckOutputArray(vtkm::cont::make_ArrayHandleReverse(array));
  }

  {
    std::cout << "ArrayHandleView" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckInputArray(
      vtkm::cont::make_ArrayHandleView(array, (ARRAY_SIZE / 3), (ARRAY_SIZE / 3) + 1));
  }

  {
    std::cout << "ArrayHandleIndex" << std::endl;
    vtkm::cont::ArrayHandleIndex array(ARRAY_SIZE);
    CheckInputArray(array, vtkm::CopyFlag::On);
  }

  {
    std::cout << "Weird combination." << std::endl;

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec4f, 2>> base0;
    base0.Allocate(ARRAY_SIZE);
    SetPortal(base0.WritePortal());

    vtkm::cont::ArrayHandleSOA<vtkm::Vec4f> base1_sub;
    base1_sub.Allocate(ARRAY_SIZE);
    SetPortal(base1_sub.WritePortal());
    auto base1 = vtkm::cont::make_ArrayHandleGroupVec<2>(base1_sub);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec4f, 2>> base2_sub;
    base2_sub.Allocate(ARRAY_SIZE + 10);
    SetPortal(base2_sub.WritePortal());
    auto base2 = vtkm::cont::make_ArrayHandleView(base2_sub, 2, ARRAY_SIZE + 4);

    auto array = vtkm::cont::make_ArrayHandleCartesianProduct(base0, base1, base2);
    CheckInputArray(array);
  }
}

} // anonymous namespace

int UnitTestArrayExtractComponent(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
