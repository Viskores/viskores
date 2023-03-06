//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/ArrayHandleXGCCoordinates.h>
#include <vtkm/cont/ArrayRangeCompute.h>

#include <vtkm/Math.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 20;

template <typename T, typename S>
void VerifyRange(const vtkm::cont::ArrayHandle<T, S>& array,
                 const vtkm::cont::ArrayHandle<vtkm::Range>& computedRangeArray)
{
  using Traits = vtkm::VecTraits<T>;
  vtkm::IdComponent numComponents = Traits::NUM_COMPONENTS;

  VTKM_TEST_ASSERT(computedRangeArray.GetNumberOfValues() == numComponents);
  auto computedRangePortal = computedRangeArray.ReadPortal();

  auto portal = array.ReadPortal();
  for (vtkm::IdComponent component = 0; component < numComponents; ++component)
  {
    vtkm::Range computedRange = computedRangePortal.Get(component);
    vtkm::Range expectedRange;
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
    {
      T value = portal.Get(index);
      expectedRange.Include(Traits::GetComponent(value, component));
    }
    VTKM_TEST_ASSERT(!vtkm::IsNan(computedRange.Min));
    VTKM_TEST_ASSERT(!vtkm::IsNan(computedRange.Max));
    VTKM_TEST_ASSERT(test_equal(expectedRange, computedRange));
  }
}

template <typename T, typename S>
void CheckRange(const vtkm::cont::ArrayHandle<T, S>& array)
{
  VerifyRange(array, vtkm::cont::ArrayRangeCompute(array));
}

template <typename T, typename S>
void FillArray(vtkm::cont::ArrayHandle<T, S>& array)
{
  using Traits = vtkm::VecTraits<T>;
  vtkm::IdComponent numComponents = Traits::NUM_COMPONENTS;

  array.AllocateAndFill(ARRAY_SIZE, vtkm::TypeTraits<T>::ZeroInitialization());

  for (vtkm::IdComponent component = 0; component < numComponents; ++component)
  {
    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float64> randomArray(ARRAY_SIZE);
    auto dest = vtkm::cont::make_ArrayHandleExtractComponent(array, component);
    vtkm::cont::ArrayCopyDevice(randomArray, dest);
  }
}

template <typename T>
void TestBasicArray()
{
  std::cout << "Checking basic array" << std::endl;
  vtkm::cont::ArrayHandleBasic<T> array;
  FillArray(array);
  CheckRange(array);
}

template <typename T>
void TestSOAArray(vtkm::TypeTraitsVectorTag)
{
  std::cout << "Checking SOA array" << std::endl;
  vtkm::cont::ArrayHandleSOA<T> array;
  FillArray(array);
  CheckRange(array);
}

template <typename T>
void TestSOAArray(vtkm::TypeTraitsScalarTag)
{
  // Skip test.
}

template <typename T>
void TestStrideArray()
{
  std::cout << "Checking stride array" << std::endl;
  vtkm::cont::ArrayHandleBasic<T> array;
  FillArray(array);
  CheckRange(vtkm::cont::ArrayHandleStride<T>(array, ARRAY_SIZE / 2, 2, 1));
}

template <typename T>
void TestCastArray()
{
  std::cout << "Checking cast array" << std::endl;
  using CastType = typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<vtkm::Float64>;
  vtkm::cont::ArrayHandle<T> array;
  FillArray(array);
  CheckRange(vtkm::cont::make_ArrayHandleCast<CastType>(array));
}

template <typename T>
void TestCartesianProduct(vtkm::TypeTraitsScalarTag)
{
  std::cout << "Checking Cartesian product" << std::endl;

  vtkm::cont::ArrayHandleBasic<T> array0;
  FillArray(array0);
  vtkm::cont::ArrayHandleBasic<T> array1;
  FillArray(array1);
  vtkm::cont::ArrayHandleBasic<T> array2;
  FillArray(array2);

  CheckRange(vtkm::cont::make_ArrayHandleCartesianProduct(array0, array1, array2));
}

template <typename T>
void TestCartesianProduct(vtkm::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
void TestComposite(vtkm::TypeTraitsScalarTag)
{
  std::cout << "Checking composite vector array" << std::endl;

  vtkm::cont::ArrayHandleBasic<T> array0;
  FillArray(array0);
  vtkm::cont::ArrayHandleBasic<T> array1;
  FillArray(array1);
  vtkm::cont::ArrayHandleBasic<T> array2;
  FillArray(array2);

  CheckRange(vtkm::cont::make_ArrayHandleCompositeVector(array0, array1, array2));
}

template <typename T>
void TestComposite(vtkm::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
void TestGroup(vtkm::TypeTraitsScalarTag)
{
  std::cout << "Checking group vec array" << std::endl;

  vtkm::cont::ArrayHandleBasic<T> array;
  FillArray(array);
  CheckRange(vtkm::cont::make_ArrayHandleGroupVec<2>(array));
}

template <typename T>
void TestGroup(vtkm::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
void TestView()
{
  std::cout << "Checking view array" << std::endl;

  vtkm::cont::ArrayHandleBasic<T> array;
  FillArray(array);
  CheckRange(vtkm::cont::make_ArrayHandleView(array, 2, ARRAY_SIZE - 5));
}

template <typename T>
void TestConstant()
{
  std::cout << "Checking constant array" << std::endl;
  CheckRange(vtkm::cont::make_ArrayHandleConstant(TestValue(10, T{}), ARRAY_SIZE));
}

template <typename T>
void TestCounting(std::true_type vtkmNotUsed(is_signed))
{
  std::cout << "Checking counting array" << std::endl;
  CheckRange(vtkm::cont::make_ArrayHandleCounting(TestValue(10, T{}), T{ 1 }, ARRAY_SIZE));

  std::cout << "Checking counting backward array" << std::endl;
  CheckRange(vtkm::cont::make_ArrayHandleCounting(TestValue(10, T{}), T{ -1 }, ARRAY_SIZE));
}

template <typename T>
void TestCounting(std::false_type vtkmNotUsed(is_signed))
{
  // Skip test
}

void TestIndex()
{
  std::cout << "Checking index array" << std::endl;
  CheckRange(vtkm::cont::make_ArrayHandleIndex(ARRAY_SIZE));
}

void TestUniformPointCoords()
{
  std::cout << "Checking uniform point coordinates" << std::endl;
  CheckRange(
    vtkm::cont::ArrayHandleUniformPointCoordinates(vtkm::Id3(ARRAY_SIZE, ARRAY_SIZE, ARRAY_SIZE)));
}

void TestXGCCoordinates()
{
  std::cout << "Checking XGC coordinates array" << std::endl;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array;
  FillArray(array);
  CheckRange(vtkm::cont::make_ArrayHandleXGCCoordinates(array, 4, true));
}

struct DoTestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    typename vtkm::TypeTraits<T>::DimensionalityTag dimensionality{};

    TestBasicArray<T>();
    TestSOAArray<T>(dimensionality);
    TestStrideArray<T>();
    TestCastArray<T>();
    TestCartesianProduct<T>(dimensionality);
    TestComposite<T>(dimensionality);
    TestGroup<T>(dimensionality);
    TestView<T>();
    TestConstant<T>();
    TestCounting<T>(typename std::is_signed<typename vtkm::VecTraits<T>::ComponentType>::type{});
  }
};

void DoTest()
{
  vtkm::testing::Testing::TryTypes(DoTestFunctor{});

  std::cout << "*** Specific arrays *****************" << std::endl;
  TestIndex();
  TestUniformPointCoords();
  TestXGCCoordinates();
}

} // anonymous namespace

int UnitTestArrayRangeCompute(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
