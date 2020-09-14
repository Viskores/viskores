//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/UncertainArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

// Make an "unusual" type to use in the test. This is simply a type that
// is sure not to be declared elsewhere.
struct UnusualType
{
  using T = vtkm::Id;
  T X;
  UnusualType() = default;
  UnusualType(T x)
    : X(x)
  {
  }
  UnusualType& operator=(T x)
  {
    this->X = x;
    return *this;
  }
  operator T() const { return this->X; }
};

} // anonymous namespace

namespace vtkm
{

// UnknownArrayHandle requires its value type to have a defined VecTraits
// class. One of the tests is to use an "unusual" array.
// Make an implementation here. Because I am lazy, this is only a partial
// implementation.
template <>
struct VecTraits<UnusualType> : VecTraits<UnusualType::T>
{
};

} // namespace vtkm

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

struct CheckFunctor
{
  template <typename T, typename S>
  static void CheckArray(const vtkm::cont::ArrayHandle<T, S>& array)
  {
    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");
    CheckPortal(array.ReadPortal());
  }

  template <typename S>
  static void CheckArray(const vtkm::cont::ArrayHandle<UnusualType, S>& array)
  {
    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");
    auto portal = array.ReadPortal();
    for (vtkm::Id index = 0; index < array.GetNumberOfValues(); ++index)
    {
      VTKM_TEST_ASSERT(portal.Get(index) == TestValue(index, UnusualType::T{}));
    }
  }

  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& array, bool& called) const
  {
    called = true;
    std::cout << "  Checking for array type " << typeid(T).name() << " with storage "
              << typeid(S).name() << std::endl;

    CheckArray(array);
  }
};

void BasicUnknownArrayChecks(const vtkm::cont::UnknownArrayHandle& array,
                             vtkm::IdComponent numComponents)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE,
                   "Dynamic array reports unexpected size.");
  VTKM_TEST_ASSERT(array.GetNumberOfComponents() == numComponents,
                   "Dynamic array reports unexpected number of components.");
}

void CheckUnknownArrayDefaults(const vtkm::cont::UnknownArrayHandle& array,
                               vtkm::IdComponent numComponents)
{
  BasicUnknownArrayChecks(array, numComponents);

  std::cout << "  CastAndCall with default types" << std::endl;
  bool called = false;
  vtkm::cont::CastAndCall(array, CheckFunctor(), called);
}

template <typename TypeList, typename StorageList>
void CheckUnknownArray(const vtkm::cont::UnknownArrayHandle& array, vtkm::IdComponent numComponents)
{
  VTKM_IS_LIST(TypeList);
  VTKM_IS_LIST(StorageList);

  BasicUnknownArrayChecks(array, numComponents);

  std::cout << "  CastAndCall with given types" << std::endl;
  bool called = false;
  array.CastAndCallForTypes<TypeList, StorageList>(CheckFunctor{}, called);
  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  std::cout << "  Check CastAndCall again with UncertainArrayHandle" << std::endl;
  called = false;
  vtkm::cont::CastAndCall(array.ResetTypes<TypeList, StorageList>(), CheckFunctor{}, called);
  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

template <typename T>
vtkm::cont::ArrayHandle<T> CreateArray(T)
{
  vtkm::cont::ArrayHandle<T> array;
  array.Allocate(ARRAY_SIZE);
  SetPortal(array.WritePortal());
  return array;
}

vtkm::cont::ArrayHandle<UnusualType> CreateArray(UnusualType)
{
  vtkm::cont::ArrayHandle<UnusualType> array;
  array.Allocate(ARRAY_SIZE);
  auto portal = array.WritePortal();
  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    portal.Set(index, TestValue(index, UnusualType::T{}));
  }
  return array;
}

template <typename T>
vtkm::cont::UnknownArrayHandle CreateArrayUnknown(T t)
{
  return vtkm::cont::UnknownArrayHandle(CreateArray(t));
}

template <typename ArrayHandleType>
void CheckAsArrayHandle(const ArrayHandleType& array)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  using T = typename ArrayHandleType::ValueType;

  vtkm::cont::UnknownArrayHandle arrayUnknown = array;
  VTKM_TEST_ASSERT(!arrayUnknown.IsType<vtkm::cont::ArrayHandle<UnusualType>>(),
                   "Dynamic array reporting is wrong type.");

  {
    std::cout << "    Normal get ArrayHandle" << std::endl;
    ArrayHandleType retreivedArray1;
    arrayUnknown.AsArrayHandle(retreivedArray1);
    VTKM_TEST_ASSERT(arrayUnknown.CanConvert<ArrayHandleType>(), "Did not query handle correctly.");
    VTKM_TEST_ASSERT(array == retreivedArray1, "Did not get back same array.");

    ArrayHandleType retreivedArray2 = arrayUnknown.AsArrayHandle<ArrayHandleType>();
    VTKM_TEST_ASSERT(array == retreivedArray2, "Did not get back same array.");
  }

  {
    std::cout << "    Put in cast array, get actual array" << std::endl;
    auto castArray = vtkm::cont::make_ArrayHandleCast<vtkm::Float64>(array);
    vtkm::cont::UnknownArrayHandle arrayUnknown2(castArray);
    VTKM_TEST_ASSERT(arrayUnknown2.IsType<ArrayHandleType>());
    ArrayHandleType retrievedArray = arrayUnknown2.AsArrayHandle<ArrayHandleType>();
    VTKM_TEST_ASSERT(array == retrievedArray);
  }

  {
    std::cout << "    Get array as cast" << std::endl;
    vtkm::cont::ArrayHandleCast<vtkm::Float64, ArrayHandleType> castArray;
    arrayUnknown.AsArrayHandle(castArray);
    VTKM_TEST_ASSERT(test_equal_portals(array.ReadPortal(), castArray.ReadPortal()));
  }

  {
    std::cout << "    Put in multiplexer, get actual array" << std::endl;
    vtkm::cont::UnknownArrayHandle arrayUnknown2 = vtkm::cont::ArrayHandleMultiplexer<
      ArrayHandleType,
      vtkm::cont::ArrayHandleConstant<typename ArrayHandleType::ValueType>>(array);
    VTKM_TEST_ASSERT(arrayUnknown2.IsType<ArrayHandleType>(),
                     "Putting in multiplexer did not pull out array.");
  }

  {
    std::cout << "    Make sure multiplex array prefers direct array (1st arg)" << std::endl;
    using MultiplexerType =
      vtkm::cont::ArrayHandleMultiplexer<ArrayHandleType,
                                         vtkm::cont::ArrayHandleCast<T, ArrayHandleType>>;
    MultiplexerType multiplexArray = arrayUnknown.AsArrayHandle<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 0);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), array.ReadPortal()));
  }

  {
    std::cout << "    Make sure multiplex array prefers direct array (2nd arg)" << std::endl;
    using MultiplexerType =
      vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandleCast<T, vtkm::cont::ArrayHandle<T>>,
                                         ArrayHandleType>;
    MultiplexerType multiplexArray = arrayUnknown.AsArrayHandle<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 1);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), array.ReadPortal()));
  }

  {
    std::cout << "    Make sure adding arrays follows nesting of special arrays" << std::endl;
    vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<vtkm::Int64>,
                                       vtkm::cont::ArrayHandleCast<vtkm::Int64, ArrayHandleType>>
      multiplexer(vtkm::cont::make_ArrayHandleCast<vtkm::Int64>(array));
    auto crazyArray = vtkm::cont::make_ArrayHandleCast<vtkm::Float64>(multiplexer);
    vtkm::cont::UnknownArrayHandle arrayUnknown2(crazyArray);
    VTKM_TEST_ASSERT(arrayUnknown2.IsType<ArrayHandleType>());
    ArrayHandleType retrievedArray = arrayUnknown2.AsArrayHandle<ArrayHandleType>();
    VTKM_TEST_ASSERT(array == retrievedArray);
  }

  {
    std::cout << "    Try adding arrays with variable amounts of components" << std::endl;
    // There might be some limited functionality, but you should still be able
    // to get arrays in and out.

    // Note, this is a bad way to implement this array. You should something like
    // ArrayHandleGroupVec instead.
    using VariableVecArrayType =
      vtkm::cont::ArrayHandleGroupVecVariable<ArrayHandleType,
                                              vtkm::cont::ArrayHandleCounting<vtkm::Id>>;
    VariableVecArrayType inArray = vtkm::cont::make_ArrayHandleGroupVecVariable(
      array, vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 2, ARRAY_SIZE / 2 + 1));
    VTKM_TEST_ASSERT(inArray.GetNumberOfValues() == ARRAY_SIZE / 2);
    vtkm::cont::UnknownArrayHandle arrayUnknown2 = inArray;
    VTKM_TEST_ASSERT(arrayUnknown2.IsType<VariableVecArrayType>());
    VariableVecArrayType retrievedArray = arrayUnknown2.AsArrayHandle<VariableVecArrayType>();
    VTKM_TEST_ASSERT(retrievedArray == inArray);
  }
}

// A vtkm::Vec if NumComps > 1, otherwise a scalar
template <typename T, vtkm::IdComponent NumComps>
using VecOrScalar = typename std::conditional<(NumComps > 1), vtkm::Vec<T, NumComps>, T>::type;

template <typename T>
void TryNewInstance(vtkm::cont::UnknownArrayHandle originalArray)
{
  // This check should already have been performed by caller, but just in case.
  CheckUnknownArray<vtkm::List<T>, VTKM_DEFAULT_STORAGE_LIST>(originalArray,
                                                              vtkm::VecTraits<T>::NUM_COMPONENTS);

  std::cout << "Create new instance of array." << std::endl;
  vtkm::cont::UnknownArrayHandle newArray = originalArray.NewInstance();

  std::cout << "Get a static instance of the new array (which checks the type)." << std::endl;
  vtkm::cont::ArrayHandle<T> staticArray;
  newArray.AsArrayHandle(staticArray);

  std::cout << "Fill the new array with invalid values and make sure the original" << std::endl
            << "is uneffected." << std::endl;
  staticArray.Allocate(ARRAY_SIZE);
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    staticArray.WritePortal().Set(index, TestValue(index + 100, T()));
  }
  CheckUnknownArray<vtkm::List<T>, VTKM_DEFAULT_STORAGE_LIST>(originalArray,
                                                              vtkm::VecTraits<T>::NUM_COMPONENTS);

  std::cout << "Set the new static array to expected values and make sure the new" << std::endl
            << "dynamic array points to the same new values." << std::endl;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    staticArray.WritePortal().Set(index, TestValue(index, T()));
  }
  CheckUnknownArray<vtkm::List<T>, VTKM_DEFAULT_STORAGE_LIST>(newArray,
                                                              vtkm::VecTraits<T>::NUM_COMPONENTS);
}

template <typename T>
void TryAsMultiplexer(vtkm::cont::UnknownArrayHandle sourceArray)
{
  auto originalArray = sourceArray.AsArrayHandle<vtkm::cont::ArrayHandle<T>>();

  {
    std::cout << "Get multiplex array through direct type." << std::endl;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<T>,
                                                               vtkm::cont::ArrayHandleConstant<T>>;
    VTKM_TEST_ASSERT(sourceArray.CanConvert<MultiplexerType>());
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }

  {
    std::cout << "Get multiplex array through cast type." << std::endl;
    using CastT = typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<vtkm::Float64>;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<
      vtkm::cont::ArrayHandle<CastT>,
      vtkm::cont::ArrayHandleCast<CastT, vtkm::cont::ArrayHandle<T>>>;
    VTKM_TEST_ASSERT(sourceArray.CanConvert<MultiplexerType>());
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }

#if 0
  // Maybe we should support this, but right now we don't
  {
    std::cout << "Make sure multiplex array prefers direct array (1st arg)" << std::endl;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<
      vtkm::cont::ArrayHandle<T>,
      vtkm::cont::ArrayHandleCast<T, vtkm::cont::ArrayHandle<T>>>;
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 0);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }

  {
    std::cout << "Make sure multiplex array prefers direct array (2nd arg)" << std::endl;
    using MultiplexerType =
      vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandleCast<T, vtkm::cont::ArrayHandle<T>>,
                                         vtkm::cont::ArrayHandle<T>>;
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VTKM_TEST_ASSERT(multiplexArray.IsValid());
    VTKM_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 1);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }
#endif
}

template <typename T>
void TryDefaultType()
{
  vtkm::cont::UnknownArrayHandle array = CreateArrayUnknown(T{});

  CheckUnknownArrayDefaults(array, vtkm::VecTraits<T>::NUM_COMPONENTS);

  TryNewInstance<T>(array);

  TryAsMultiplexer<T>(array);
}

struct TryBasicVTKmType
{
  template <typename T>
  void operator()(T) const
  {
    vtkm::cont::UnknownArrayHandle array = CreateArrayUnknown(T());

    CheckUnknownArray<vtkm::TypeListAll, VTKM_DEFAULT_STORAGE_LIST>(
      array, vtkm::VecTraits<T>::NUM_COMPONENTS);

    TryNewInstance<T>(array);
  }
};

void TryUnusualType()
{
  // A string is an unlikely type to be declared elsewhere in VTK-m.
  vtkm::cont::UnknownArrayHandle array = CreateArrayUnknown(UnusualType{});

  try
  {
    CheckUnknownArray<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(array, 1);
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized type.");
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    std::cout << "  Caught exception for unrecognized type." << std::endl;
  }
  CheckUnknownArray<vtkm::List<UnusualType>, VTKM_DEFAULT_STORAGE_LIST>(array, 1);
  std::cout << "  Found type when type list was reset." << std::endl;
}

template <typename ArrayHandleType>
void TryAsArrayHandle(const ArrayHandleType& array)
{
  CheckAsArrayHandle(array);
}

void TryAsArrayHandle()
{
  std::cout << "  Normal array handle." << std::endl;
  vtkm::Id buffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index, vtkm::Id());
  }

  vtkm::cont::ArrayHandle<vtkm::Id> array =
    vtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE, vtkm::CopyFlag::On);
  TryAsArrayHandle(array);

  std::cout << "  Constant array handle." << std::endl;
  TryAsArrayHandle(vtkm::cont::make_ArrayHandleConstant(5, ARRAY_SIZE));
}

void TrySetCastArray()
{
  vtkm::cont::ArrayHandle<vtkm::Id> knownArray = CreateArray(vtkm::Id{});
  vtkm::cont::UnknownArrayHandle unknownArray(
    vtkm::cont::make_ArrayHandleCast<vtkm::Float32>(knownArray));

  // The unknownArray should actually hold the original knownArray type even though we gave it
  // a cast array.
  CheckUnknownArray<vtkm::List<vtkm::Id>, vtkm::List<VTKM_DEFAULT_STORAGE_TAG>>(unknownArray, 1);
}

void TrySetMultiplexerArray()
{
  vtkm::cont::ArrayHandle<vtkm::Id> knownArray = CreateArray(vtkm::Id{});
  vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<vtkm::Id>,
                                     vtkm::cont::ArrayHandleConstant<vtkm::Id>>
    multiplexerArray(knownArray);
  vtkm::cont::UnknownArrayHandle unknownArray(multiplexerArray);

  // The unknownArray should actually hold the original knownArray type even though we gave it
  // a multiplexer array.
  CheckUnknownArray<vtkm::List<vtkm::Id>, vtkm::List<VTKM_DEFAULT_STORAGE_TAG>>(unknownArray, 1);
}

struct DefaultTypeFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TryDefaultType<T>();
  }
};

void TestUnknownArrayHandle()
{
  std::cout << "Try common types with default type lists." << std::endl;
  vtkm::testing::Testing::TryTypes(DefaultTypeFunctor{}, VTKM_DEFAULT_TYPE_LIST{});

  std::cout << "Try exemplar VTK-m types." << std::endl;
  vtkm::testing::Testing::TryTypes(TryBasicVTKmType{});

  std::cout << "Try unusual type." << std::endl;
  TryUnusualType();

  std::cout << "Try AsArrayHandle" << std::endl;
  TryAsArrayHandle();

  std::cout << "Try setting ArrayHandleCast" << std::endl;
  TrySetCastArray();

  std::cout << "Try setting ArrayHandleMultiplexer" << std::endl;
  TrySetMultiplexerArray();
}

} // anonymous namespace

int UnitTestUnknownArrayHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestUnknownArrayHandle, argc, argv);
}
