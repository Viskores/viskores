//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/VariantArrayHandle.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtual.h>
#include <vtkm/cont/ArrayHandleZip.h>

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/cont/testing/Testing.h>

#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace vtkm
{

// VariantArrayHandle requires its value type to have a defined VecTraits
// class. One of the tests is to use an "unusual" array of std::string
// (which is pretty pointless but might tease out some assumptions).
// Make an implementation here. Because I am lazy, this is only a partial
// implementation.
template <>
struct VecTraits<std::string>
{
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 1;
  using HasMultipleComponents = vtkm::VecTraitsTagSingleComponent;
};

} // namespace vtkm

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

using TypeListString = vtkm::List<std::string>;

template <typename T>
struct UnusualPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return TestValue(index, ValueType()); }
};

template <typename T>
class ArrayHandleWithUnusualStorage
  : public vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagImplicit<UnusualPortal<T>>>
{
  using Superclass = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagImplicit<UnusualPortal<T>>>;

public:
  VTKM_CONT
  ArrayHandleWithUnusualStorage()
    : Superclass(typename Superclass::PortalConstControl())
  {
  }
};

template <typename T>
struct TestValueFunctor
{
  T operator()(vtkm::Id index) const { return TestValue(index, T()); }
};

struct CheckFunctor
{
  template <typename T>
  void operator()(const vtkm::cont::ArrayHandle<T>& array,
                  bool& calledBasic,
                  bool& vtkmNotUsed(calledUnusual),
                  bool& vtkmNotUsed(calledVirtual)) const
  {
    calledBasic = true;
    std::cout << "  Checking for basic array type: " << typeid(T).name() << std::endl;

    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");

    auto portal = array.GetPortalConstControl();
    CheckPortal(portal);
  }

  template <typename T>
  void operator()(
    const vtkm::cont::ArrayHandle<T, typename ArrayHandleWithUnusualStorage<T>::StorageTag>& array,
    bool& vtkmNotUsed(calledBasic),
    bool& calledUnusual,
    bool& vtkmNotUsed(calledVirtual)) const
  {
    calledUnusual = true;
    std::cout << "  Checking for unusual array type: " << typeid(T).name() << std::endl;

    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");

    auto portal = array.GetPortalConstControl();
    CheckPortal(portal);
  }

  template <typename T>
  void operator()(const vtkm::cont::ArrayHandleVirtual<T>& array,
                  bool& vtkmNotUsed(calledBasic),
                  bool& vtkmNotUsed(calledUnusual),
                  bool& calledVirtual) const
  {
    calledVirtual = true;
    std::cout << "  Checking for virtual array type: " << typeid(T).name() << std::endl;

    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");

    auto portal = array.GetPortalConstControl();
    CheckPortal(portal);
  }
};

template <typename TypeList>
void BasicArrayVariantChecks(const vtkm::cont::VariantArrayHandleBase<TypeList>& array,
                             vtkm::IdComponent numComponents)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE,
                   "Dynamic array reports unexpected size.");
  std::cout << "array.GetNumberOfComponents() = " << array.GetNumberOfComponents() << ", "
            << "numComponents = " << numComponents << "\n";
  VTKM_TEST_ASSERT(array.GetNumberOfComponents() == numComponents,
                   "Dynamic array reports unexpected number of components.");
}

template <typename TypeList>
void CheckArrayVariant(const vtkm::cont::VariantArrayHandleBase<TypeList>& array,
                       vtkm::IdComponent numComponents,
                       bool isBasicArray,
                       bool isUnusualArray)
{
  BasicArrayVariantChecks(array, numComponents);

  std::cout << "  CastAndCall with default storage" << std::endl;
  bool calledBasic = false;
  bool calledUnusual = false;
  bool calledVirtual = false;
  CastAndCall(array, CheckFunctor(), calledBasic, calledUnusual, calledVirtual);

  VTKM_TEST_ASSERT(
    calledBasic || calledUnusual || calledVirtual,
    "The functor was never called (and apparently a bad value exception not thrown).");
  if (isBasicArray)
  {
    VTKM_TEST_ASSERT(calledBasic, "The functor was never called with the basic array fast path");
    VTKM_TEST_ASSERT(!calledUnusual, "The functor was somehow called with the unusual fast path");
    VTKM_TEST_ASSERT(!calledVirtual, "The functor was somehow called with the virtual path");
  }
  else
  {
    VTKM_TEST_ASSERT(!calledBasic, "The array somehow got cast to a basic storage.");
    VTKM_TEST_ASSERT(!calledUnusual, "The array somehow got cast to an unusual storage.");
  }

  std::cout << "  CastAndCall with no storage" << std::endl;
  calledBasic = false;
  calledUnusual = false;
  calledVirtual = false;
  array.CastAndCall(vtkm::ListEmpty(), CheckFunctor(), calledBasic, calledUnusual, calledVirtual);
  VTKM_TEST_ASSERT(
    calledBasic || calledUnusual || calledVirtual,
    "The functor was never called (and apparently a bad value exception not thrown).");
  VTKM_TEST_ASSERT(!calledBasic, "The array somehow got cast to a basic storage.");
  VTKM_TEST_ASSERT(!calledUnusual, "The array somehow got cast to an unusual storage.");

  std::cout << "  CastAndCall with extra storage" << std::endl;
  calledBasic = false;
  calledUnusual = false;
  calledVirtual = false;
  array.CastAndCall(vtkm::List<vtkm::cont::StorageTagBasic,
                               ArrayHandleWithUnusualStorage<vtkm::Id>::StorageTag,
                               ArrayHandleWithUnusualStorage<std::string>::StorageTag>(),
                    CheckFunctor(),
                    calledBasic,
                    calledUnusual,
                    calledVirtual);
  VTKM_TEST_ASSERT(
    calledBasic || calledUnusual || calledVirtual,
    "The functor was never called (and apparently a bad value exception not thrown).");
  if (isBasicArray)
  {
    VTKM_TEST_ASSERT(calledBasic, "The functor was never called with the basic array fast path");
    VTKM_TEST_ASSERT(!calledUnusual, "The functor was somehow called with the unusual fast path");
    VTKM_TEST_ASSERT(!calledVirtual, "The functor was somehow called with the virtual path");
  }
  else if (isUnusualArray)
  {
    VTKM_TEST_ASSERT(calledUnusual, "The functor was never called with the unusual fast path");
    VTKM_TEST_ASSERT(!calledBasic, "The functor was somehow called with the basic fast path");
    VTKM_TEST_ASSERT(!calledVirtual, "The functor was somehow called with the virtual path");
  }
  else
  {
    VTKM_TEST_ASSERT(!calledBasic, "The array somehow got cast to a basic storage.");
    VTKM_TEST_ASSERT(!calledUnusual, "The array somehow got cast to an unusual storage.");
  }
}

template <typename T>
vtkm::cont::VariantArrayHandle CreateArrayVariant(T)
{
  // Declared static to prevent going out of scope.
  static T buffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index, T());
  }

  return vtkm::cont::VariantArrayHandle(vtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE));
}

template <typename ArrayHandleType>
void CheckCastToArrayHandle(const ArrayHandleType& array)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  vtkm::cont::VariantArrayHandle arrayVariant = array;
  VTKM_TEST_ASSERT(!arrayVariant.IsType<vtkm::cont::ArrayHandle<std::string>>(),
                   "Dynamic array reporting is wrong type.");

  ArrayHandleType castArray1;
  arrayVariant.CopyTo(castArray1);
  VTKM_TEST_ASSERT(arrayVariant.IsType<ArrayHandleType>(), "Did not query handle correctly.");
  VTKM_TEST_ASSERT(array == castArray1, "Did not get back same array.");

  ArrayHandleType castArray2 = arrayVariant.Cast<ArrayHandleType>();
  VTKM_TEST_ASSERT(array == castArray2, "Did not get back same array.");
}

// A vtkm::Vec if NumComps > 1, otherwise a scalar
template <typename T, vtkm::IdComponent NumComps>
using VecOrScalar = typename std::conditional<(NumComps > 1), vtkm::Vec<T, NumComps>, T>::type;

template <typename ArrayType>
void CheckCastToVirtualArrayHandle(const ArrayType& array)
{
  VTKM_IS_ARRAY_HANDLE(ArrayType);

  using ValueType = typename ArrayType::ValueType;
  using VTraits = vtkm::VecTraits<ValueType>;
  using ComponentType = typename VTraits::ComponentType;
  static constexpr vtkm::IdComponent NumComps = VTraits::NUM_COMPONENTS;

  using Storage = typename ArrayType::StorageTag;
  using StorageList = vtkm::ListAppend<VTKM_DEFAULT_STORAGE_LIST, vtkm::List<Storage>>;

  using TypeList = vtkm::ListAppend<VTKM_DEFAULT_TYPE_LIST, vtkm::List<ValueType>>;
  using VariantArrayType = vtkm::cont::VariantArrayHandleBase<TypeList>;

  VariantArrayType arrayVariant = array;

  {
    auto testArray = arrayVariant.template AsVirtual<ValueType, StorageList>();
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == array.GetNumberOfValues(),
                     "Did not get back virtual array handle representation.");
  }

  {
    auto testArray =
      arrayVariant.template AsVirtual<VecOrScalar<vtkm::Int8, NumComps>, StorageList>();
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == array.GetNumberOfValues(),
                     "Did not get back virtual array handle representation.");
  }

  {
    auto testArray =
      arrayVariant.template AsVirtual<VecOrScalar<vtkm::Int64, NumComps>, StorageList>();
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == array.GetNumberOfValues(),
                     "Did not get back virtual array handle representation.");
  }

  {
    auto testArray =
      arrayVariant.template AsVirtual<VecOrScalar<vtkm::UInt64, NumComps>, StorageList>();
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == array.GetNumberOfValues(),
                     "Did not get back virtual array handle representation.");
  }

  {
    auto testArray =
      arrayVariant.template AsVirtual<VecOrScalar<vtkm::Float32, NumComps>, StorageList>();
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == array.GetNumberOfValues(),
                     "Did not get back virtual array handle representation.");
  }

  {
    auto testArray =
      arrayVariant.template AsVirtual<VecOrScalar<vtkm::Float64, NumComps>, StorageList>();
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == array.GetNumberOfValues(),
                     "Did not get back virtual array handle representation.");
  }

  bool threw = false;
  try
  {
    arrayVariant.template AsVirtual<vtkm::Vec<ComponentType, NumComps + 1>, StorageList>();
  }
  catch (vtkm::cont::ErrorBadType&)
  {
    // caught expected exception
    threw = true;
  }

  VTKM_TEST_ASSERT(threw,
                   "Casting to different vector width did not throw expected "
                   "ErrorBadType exception.");
}

template <typename T, typename ArrayVariantType>
void TryNewInstance(T, ArrayVariantType originalArray)
{
  // This check should already have been performed by caller, but just in case.
  CheckArrayVariant(originalArray, vtkm::VecTraits<T>::NUM_COMPONENTS, true, false);

  std::cout << "Create new instance of array." << std::endl;
  ArrayVariantType newArray = originalArray.NewInstance();

  std::cout << "Get a static instance of the new array (which checks the type)." << std::endl;
  vtkm::cont::ArrayHandle<T> staticArray;
  newArray.CopyTo(staticArray);

  std::cout << "Fill the new array with invalid values and make sure the original" << std::endl
            << "is uneffected." << std::endl;
  staticArray.Allocate(ARRAY_SIZE);
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    staticArray.GetPortalControl().Set(index, TestValue(index + 100, T()));
  }
  CheckArrayVariant(originalArray, vtkm::VecTraits<T>::NUM_COMPONENTS, true, false);

  std::cout << "Set the new static array to expected values and make sure the new" << std::endl
            << "dynamic array points to the same new values." << std::endl;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    staticArray.GetPortalControl().Set(index, TestValue(index, T()));
  }
  CheckArrayVariant(newArray, vtkm::VecTraits<T>::NUM_COMPONENTS, true, false);
}

template <typename T, typename ArrayVariantType>
void TryAsMultiplexer(T, ArrayVariantType sourceArray)
{
  auto originalArray = sourceArray.template Cast<vtkm::cont::ArrayHandle<T>>();

  {
    std::cout << "Get multiplex array through direct type." << std::endl;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<T>,
                                                               vtkm::cont::ArrayHandleConstant<T>>;
    MultiplexerType multiplexArray = sourceArray.template AsMultiplexer<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.GetPortalConstControl(),
                                        originalArray.GetPortalConstControl()));
  }

  {
    std::cout << "Get multiplex array through cast type." << std::endl;
    using CastT = typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<vtkm::Float64>;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<
      vtkm::cont::ArrayHandle<CastT>,
      vtkm::cont::ArrayHandleCast<CastT, vtkm::cont::ArrayHandle<T>>>;
    MultiplexerType multiplexArray = sourceArray.template AsMultiplexer<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.GetPortalConstControl(),
                                        originalArray.GetPortalConstControl()));
  }

  {
    std::cout << "Make sure multiplex array prefers direct array (1st arg)" << std::endl;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<
      vtkm::cont::ArrayHandle<T>,
      vtkm::cont::ArrayHandleCast<T, vtkm::cont::ArrayHandle<T>>>;
    MultiplexerType multiplexArray = sourceArray.template AsMultiplexer<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 0);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.GetPortalConstControl(),
                                        originalArray.GetPortalConstControl()));
  }

  {
    std::cout << "Make sure multiplex array prefers direct array (2nd arg)" << std::endl;
    using MultiplexerType =
      vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandleCast<T, vtkm::cont::ArrayHandle<T>>,
                                         vtkm::cont::ArrayHandle<T>>;
    MultiplexerType multiplexArray = sourceArray.template AsMultiplexer<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 1);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.GetPortalConstControl(),
                                        originalArray.GetPortalConstControl()));
  }
}

template <typename T>
void TryDefaultType(T)
{
  vtkm::cont::VariantArrayHandle array = CreateArrayVariant(T());

  CheckArrayVariant(array, vtkm::VecTraits<T>::NUM_COMPONENTS, true, false);

  TryNewInstance(T(), array);

  TryAsMultiplexer(T(), array);
}

struct TryBasicVTKmType
{
  template <typename T>
  void operator()(T) const
  {
    vtkm::cont::VariantArrayHandle array = CreateArrayVariant(T());

    CheckArrayVariant(
      array.ResetTypes(vtkm::TypeListAll()), vtkm::VecTraits<T>::NUM_COMPONENTS, true, false);

    TryNewInstance(T(), array.ResetTypes(vtkm::TypeListAll()));
  }
};

void TryUnusualType()
{
  // A string is an unlikely type to be declared elsewhere in VTK-m.
  vtkm::cont::VariantArrayHandle array = CreateArrayVariant(std::string());

  try
  {
    CheckArrayVariant(array, 1, true, false);
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized type.");
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    std::cout << "  Caught exception for unrecognized type." << std::endl;
  }

  CheckArrayVariant(array.ResetTypes(TypeListString()), 1, true, false);
  std::cout << "  Found type when type list was reset." << std::endl;
}

void TryUnusualStorage()
{
  vtkm::cont::VariantArrayHandle array = ArrayHandleWithUnusualStorage<vtkm::Id>();

  try
  {
    CheckArrayVariant(array, 1, false, true);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("CastAndCall with Variant failed to handle unusual storage.");
  }
}

void TryUnusualTypeAndStorage()
{
  vtkm::cont::VariantArrayHandle array = ArrayHandleWithUnusualStorage<std::string>();

  try
  {
    CheckArrayVariant(array, 1, false, true);
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized type/storage.");
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    std::cout << "  Caught exception for unrecognized type/storage." << std::endl;
  }

  try
  {
    CheckArrayVariant(array.ResetTypes(TypeListString()), 1, false, true);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("CastAndCall with Variant failed to handle unusual storage.");
  }
}

template <typename ArrayHandleType>
void TryCastToArrayHandle(const ArrayHandleType& array)
{
  CheckCastToArrayHandle(array);
  CheckCastToVirtualArrayHandle(array);
}

void TryCastToArrayHandle()
{
  std::cout << "  Normal array handle." << std::endl;
  vtkm::Id buffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index, vtkm::Id());
  }

  vtkm::cont::ArrayHandle<vtkm::Id> array = vtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE);
  TryCastToArrayHandle(array);

  std::cout << "  Cast array handle." << std::endl;
  TryCastToArrayHandle(vtkm::cont::make_ArrayHandleCast(array, vtkm::FloatDefault()));

  std::cout << "  Composite vector array handle." << std::endl;
  TryCastToArrayHandle(vtkm::cont::make_ArrayHandleCompositeVector(array, array));

  std::cout << "  Constant array handle." << std::endl;
  TryCastToArrayHandle(vtkm::cont::make_ArrayHandleConstant(5, ARRAY_SIZE));

  std::cout << "  Counting array handle." << std::endl;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> countingArray(ARRAY_SIZE - 1, -1, ARRAY_SIZE);
  TryCastToArrayHandle(countingArray);

  std::cout << "  Group vec array handle" << std::endl;
  vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<vtkm::Id>, 2> groupVecArray(array);
  TryCastToArrayHandle(groupVecArray);

  std::cout << "  Implicit array handle." << std::endl;
  TryCastToArrayHandle(
    vtkm::cont::make_ArrayHandleImplicit(TestValueFunctor<vtkm::FloatDefault>(), ARRAY_SIZE));

  std::cout << "  Index array handle." << std::endl;
  TryCastToArrayHandle(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE));

  std::cout << "  Permutation array handle." << std::endl;
  TryCastToArrayHandle(vtkm::cont::make_ArrayHandlePermutation(countingArray, array));

  std::cout << "  Transform array handle." << std::endl;
  TryCastToArrayHandle(
    vtkm::cont::make_ArrayHandleTransform(countingArray, TestValueFunctor<vtkm::FloatDefault>()));

  std::cout << "  Uniform point coordinates array handle." << std::endl;
  TryCastToArrayHandle(vtkm::cont::ArrayHandleUniformPointCoordinates(vtkm::Id3(ARRAY_SIZE)));

  // std::cout << "  Zip array handle." << std::endl;
  // CheckCastToArrayHandle(vtkm::cont::make_ArrayHandleZip(countingArray, array));
}

void TestVariantArrayHandle()
{
  std::cout << "Try common types with default type lists." << std::endl;
  std::cout << "*** vtkm::Id **********************" << std::endl;
  TryDefaultType(vtkm::Id());
  std::cout << "*** vtkm::FloatDefault ************" << std::endl;
  TryDefaultType(vtkm::FloatDefault());
  std::cout << "*** vtkm::Float32 *****************" << std::endl;
  TryDefaultType(vtkm::Float32());
  std::cout << "*** vtkm::Float64 *****************" << std::endl;
  TryDefaultType(vtkm::Float64());
  std::cout << "*** vtkm::Vec<Float32,3> **********" << std::endl;
  TryDefaultType(vtkm::Vec3f_32());
  std::cout << "*** vtkm::Vec<Float64,3> **********" << std::endl;
  TryDefaultType(vtkm::Vec3f_64());

  std::cout << "Try exemplar VTK-m types." << std::endl;
  vtkm::testing::Testing::TryTypes(TryBasicVTKmType());

  std::cout << "Try unusual type." << std::endl;
  TryUnusualType();

  std::cout << "Try unusual storage." << std::endl;
  TryUnusualStorage();

  std::cout << "Try unusual type in unusual storage." << std::endl;
  TryUnusualTypeAndStorage();

  std::cout << "Try CastToArrayHandle" << std::endl;
  TryCastToArrayHandle();
}

} // anonymous namespace

int UnitTestVariantArrayHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVariantArrayHandle, argc, argv);
}
