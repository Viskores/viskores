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
void VerifyRangeScalar(const vtkm::cont::ArrayHandle<T, S>& array,
                       const vtkm::cont::ArrayHandle<vtkm::Range>& computedRangeArray,
                       const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
                       bool finitesOnly)
{
  using Traits = vtkm::VecTraits<T>;
  vtkm::IdComponent numComponents = Traits::NUM_COMPONENTS;

  VTKM_TEST_ASSERT(computedRangeArray.GetNumberOfValues() == numComponents);
  auto computedRangePortal = computedRangeArray.ReadPortal();

  auto portal = array.ReadPortal();
  auto maskPortal = maskArray.ReadPortal();
  for (vtkm::IdComponent component = 0; component < numComponents; ++component)
  {
    vtkm::Range computedRange = computedRangePortal.Get(component);
    vtkm::Range expectedRange{};
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
    {
      if (maskPortal.GetNumberOfValues() != 0 && (maskPortal.Get(index) == 0))
      {
        continue;
      }
      auto value = static_cast<vtkm::Float64>(Traits::GetComponent(portal.Get(index), component));
      if (finitesOnly && !vtkm::IsFinite(value))
      {
        continue;
      }
      expectedRange.Include(value);
    }
    try
    {
      VTKM_TEST_ASSERT(!vtkm::IsNan(computedRange.Min));
      VTKM_TEST_ASSERT(!vtkm::IsNan(computedRange.Max));
      VTKM_TEST_ASSERT((!expectedRange.IsNonEmpty() && !computedRange.IsNonEmpty()) ||
                       (test_equal(expectedRange, computedRange)));
    }
    catch (const vtkm::testing::Testing::TestFailure&)
    {
      std::cout << "Test array: \n";
      vtkm::cont::printSummary_ArrayHandle(array, std::cout, true);
      std::cout << "Mask array: \n";
      vtkm::cont::printSummary_ArrayHandle(maskArray, std::cout, true);
      std::cout << "Range type: " << (finitesOnly ? "Scalar, Finite" : "Scalar, NonFinite") << "\n";
      std::cout << "Computed range: \n";
      vtkm::cont::printSummary_ArrayHandle(computedRangeArray, std::cout, true);
      std::cout << "Expected range: " << expectedRange << ", component: " << component << "\n";
      throw;
    }
  }
}

template <typename T, typename S>
void VerifyRangeVector(const vtkm::cont::ArrayHandle<T, S>& array,
                       const vtkm::Range& computedRange,
                       const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
                       bool finitesOnly)
{
  auto portal = array.ReadPortal();
  auto maskPortal = maskArray.ReadPortal();
  vtkm::Range expectedRange{};
  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    if (maskPortal.GetNumberOfValues() != 0 && (maskPortal.Get(index) == 0))
    {
      continue;
    }
    auto value = static_cast<vtkm::Float64>(vtkm::MagnitudeSquared(portal.Get(index)));
    if (finitesOnly && !vtkm::IsFinite(value))
    {
      continue;
    }
    expectedRange.Include(value);
  }

  if (expectedRange.IsNonEmpty())
  {
    expectedRange.Min = vtkm::Sqrt(expectedRange.Min);
    expectedRange.Max = vtkm::Sqrt(expectedRange.Max);
  }

  try
  {
    VTKM_TEST_ASSERT(!vtkm::IsNan(computedRange.Min));
    VTKM_TEST_ASSERT(!vtkm::IsNan(computedRange.Max));
    VTKM_TEST_ASSERT((!expectedRange.IsNonEmpty() && !computedRange.IsNonEmpty()) ||
                     (test_equal(expectedRange, computedRange)));
  }
  catch (const vtkm::testing::Testing::TestFailure&)
  {
    std::cout << "Test array: \n";
    vtkm::cont::printSummary_ArrayHandle(array, std::cout, true);
    std::cout << "Mask array: \n";
    vtkm::cont::printSummary_ArrayHandle(maskArray, std::cout, true);
    std::cout << "Range type: " << (finitesOnly ? "Vector, Finite" : "Vector, NonFinite") << "\n";
    std::cout << "Computed range: " << computedRange << "\n";
    std::cout << "Expected range: " << expectedRange << "\n";
    throw;
  }
}

auto FillMaskArray(vtkm::Id length)
{
  vtkm::cont::ArrayHandle<vtkm::UInt8> maskArray;
  maskArray.Allocate(length);

  vtkm::cont::ArrayHandleRandomUniformBits randomBits(length + 1);
  auto randomPortal = randomBits.ReadPortal();
  switch (randomPortal.Get(length) % 3)
  {
    case 0: // all masked
      maskArray.Fill(0);
      break;
    case 1: // none masked
      maskArray.Fill(1);
      break;
    case 2: // random masked
    default:
    {
      auto maskPortal = maskArray.WritePortal();
      for (vtkm::Id i = 0; i < length; ++i)
      {
        vtkm::UInt8 maskVal = ((randomPortal.Get(i) % 8) == 0) ? 0 : 1;
        maskPortal.Set(i, maskVal);
      }
      break;
    }
  }

  return maskArray;
}

template <typename T, typename S>
void CheckRange(const vtkm::cont::ArrayHandle<T, S>& array)
{
  auto length = array.GetNumberOfValues();
  vtkm::cont::ArrayHandle<vtkm::UInt8> emptyMaskArray;

  auto maskArray = FillMaskArray(length);

  vtkm::cont::ArrayHandle<vtkm::Range> scalarRange;
  std::cout << "\tchecking scalar range without mask\n";
  scalarRange = vtkm::cont::ArrayRangeCompute(array);
  VerifyRangeScalar(array, scalarRange, emptyMaskArray, false);
  std::cout << "\tchecking scalar range with mask\n";
  scalarRange = vtkm::cont::ArrayRangeCompute(array, maskArray);
  VerifyRangeScalar(array, scalarRange, maskArray, false);

  vtkm::Range vectorRange;
  std::cout << "\tchecking vector range without mask\n";
  vectorRange = vtkm::cont::ArrayRangeComputeMagnitude(array);
  VerifyRangeVector(array, vectorRange, emptyMaskArray, false);
  std::cout << "\tchecking vector range with mask\n";
  vectorRange = vtkm::cont::ArrayRangeComputeMagnitude(array, maskArray);
  VerifyRangeVector(array, vectorRange, maskArray, false);
}

template <typename ArrayHandleType>
void CheckRangeFiniteImpl(const ArrayHandleType& array, std::true_type)
{
  auto length = array.GetNumberOfValues();
  vtkm::cont::ArrayHandle<vtkm::UInt8> emptyMaskArray;

  auto maskArray = FillMaskArray(length);

  vtkm::cont::ArrayHandle<vtkm::Range> scalarRange;
  std::cout << "\tchecking finite scalar range without mask\n";
  scalarRange = vtkm::cont::ArrayRangeCompute(array, true);
  VerifyRangeScalar(array, scalarRange, emptyMaskArray, true);
  std::cout << "\tchecking finite scalar range with mask\n";
  scalarRange = vtkm::cont::ArrayRangeCompute(array, maskArray, true);
  VerifyRangeScalar(array, scalarRange, maskArray, true);

  vtkm::Range vectorRange;
  std::cout << "\tchecking finite vector range without mask\n";
  vectorRange = vtkm::cont::ArrayRangeComputeMagnitude(array, true);
  VerifyRangeVector(array, vectorRange, emptyMaskArray, true);
  std::cout << "\tchecking finite vector range with mask\n";
  vectorRange = vtkm::cont::ArrayRangeComputeMagnitude(array, maskArray, true);
  VerifyRangeVector(array, vectorRange, maskArray, true);
}

template <typename ArrayHandleType>
void CheckRangeFiniteImpl(const ArrayHandleType&, std::false_type)
{
}

template <typename T, typename S>
void CheckRangeFinite(const vtkm::cont::ArrayHandle<T, S>& array)
{
  using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
  auto tag = std::integral_constant < bool,
       std::is_same<ComponentType, vtkm::Float32>::value ||
    std::is_same<ComponentType, vtkm::Float64>::value > {};
  CheckRangeFiniteImpl(array, tag);
}

// Transform random values in range [0, 1) to the range [From, To).
// If the `AddNonFinites` flag is set, some values are transformed to non-finite values.
struct TransformRange
{
  vtkm::Float64 From, To;
  bool AddNonFinites = false;

  VTKM_EXEC vtkm::Float64 operator()(vtkm::Float64 in) const
  {
    if (AddNonFinites)
    {
      if (in >= 0.3 && in <= 0.33)
      {
        return vtkm::NegativeInfinity64();
      }
      if (in >= 0.9 && in <= 0.93)
      {
        return vtkm::Infinity64();
      }
    }
    return (in * (this->To - this->From)) + this->From;
  }
};

template <typename T, typename S>
void FillArray(vtkm::cont::ArrayHandle<T, S>& array, bool addNonFinites)
{
  using Traits = vtkm::VecTraits<T>;
  vtkm::IdComponent numComponents = Traits::NUM_COMPONENTS;

  // non-finites only applies to floating point types
  addNonFinites = addNonFinites &&
    (std::is_same<typename Traits::ComponentType, vtkm::Float32>::value ||
     std::is_same<typename Traits::ComponentType, vtkm::Float64>::value);

  array.AllocateAndFill(ARRAY_SIZE, vtkm::TypeTraits<T>::ZeroInitialization());

  for (vtkm::IdComponent component = 0; component < numComponents; ++component)
  {
    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float64> randomArray(ARRAY_SIZE);
    auto dest = vtkm::cont::make_ArrayHandleExtractComponent(array, component);
    auto transformFunctor = std::numeric_limits<typename Traits::BaseComponentType>::is_signed
      ? TransformRange{ -100.0, 100.0, addNonFinites }
      : TransformRange{ 0.0, 200.0, addNonFinites };
    vtkm::cont::ArrayCopyDevice(
      vtkm::cont::make_ArrayHandleTransform(randomArray, transformFunctor), dest);
  }
}

template <typename T>
void TestBasicArray()
{
  std::cout << "Checking basic array" << std::endl;
  vtkm::cont::ArrayHandleBasic<T> array;
  FillArray(array, false);
  CheckRange(array);
  FillArray(array, true);
  CheckRangeFinite(array);
}

template <typename T>
void TestSOAArray(vtkm::TypeTraitsVectorTag)
{
  std::cout << "Checking SOA array" << std::endl;
  vtkm::cont::ArrayHandleSOA<T> array;
  FillArray(array, false);
  CheckRange(array);
  FillArray(array, true);
  CheckRangeFinite(array);
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
  FillArray(array, false);
  CheckRange(vtkm::cont::ArrayHandleStride<T>(array, ARRAY_SIZE / 2, 2, 1));
  FillArray(array, true);
  CheckRangeFinite(vtkm::cont::ArrayHandleStride<T>(array, ARRAY_SIZE / 2, 2, 1));
}

template <typename T>
void TestCastArray()
{
  std::cout << "Checking cast array" << std::endl;
  using CastType = typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<vtkm::Float64>;
  vtkm::cont::ArrayHandle<T> array;
  FillArray(array, false);
  CheckRange(vtkm::cont::make_ArrayHandleCast<CastType>(array));
}

template <typename T>
auto FillCartesianProductArray(bool addNonFinites)
{
  vtkm::cont::ArrayHandleBasic<T> array0;
  FillArray(array0, addNonFinites);
  vtkm::cont::ArrayHandleBasic<T> array1;
  FillArray(array1, addNonFinites);
  vtkm::cont::ArrayHandleBasic<T> array2;
  FillArray(array2, addNonFinites);
  return vtkm::cont::make_ArrayHandleCartesianProduct(array0, array1, array2);
}

template <typename T>
void TestCartesianProduct(vtkm::TypeTraitsScalarTag)
{
  std::cout << "Checking Cartesian product" << std::endl;
  auto array = FillCartesianProductArray<T>(false);
  CheckRange(array);
  array = FillCartesianProductArray<T>(true);
  CheckRangeFinite(array);
}

template <typename T>
void TestCartesianProduct(vtkm::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
auto FillCompositeVectorArray(bool addNonFinites)
{
  vtkm::cont::ArrayHandleBasic<T> array0;
  FillArray(array0, addNonFinites);
  vtkm::cont::ArrayHandleBasic<T> array1;
  FillArray(array1, addNonFinites);
  vtkm::cont::ArrayHandleBasic<T> array2;
  FillArray(array2, addNonFinites);
  return vtkm::cont::make_ArrayHandleCompositeVector(array0, array1, array2);
}

template <typename T>
void TestComposite(vtkm::TypeTraitsScalarTag)
{
  std::cout << "Checking composite vector array" << std::endl;

  auto array = FillCompositeVectorArray<T>(false);
  CheckRange(array);
  array = FillCompositeVectorArray<T>(true);
  CheckRangeFinite(array);
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
  FillArray(array, false);
  CheckRange(vtkm::cont::make_ArrayHandleGroupVec<2>(array));
  FillArray(array, true);
  CheckRangeFinite(vtkm::cont::make_ArrayHandleGroupVec<2>(array));
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
  FillArray(array, false);
  CheckRange(vtkm::cont::make_ArrayHandleView(array, 2, ARRAY_SIZE - 5));
  FillArray(array, true);
  CheckRangeFinite(vtkm::cont::make_ArrayHandleView(array, 2, ARRAY_SIZE - 5));
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
  FillArray(array, false);
  CheckRange(vtkm::cont::make_ArrayHandleXGCCoordinates(array, 4, true));
  FillArray(array, true);
  CheckRangeFinite(vtkm::cont::make_ArrayHandleXGCCoordinates(array, 4, true));
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
