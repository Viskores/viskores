//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================


#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/StorageImplicit.h>

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/cont/testing/Testing.h>

#include <sstream>
#include <string>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

vtkm::Id TestValue(vtkm::Id index, vtkm::Id) {
  return index*100;
}

vtkm::Scalar TestValue(vtkm::Id index, vtkm::Scalar) {
  return static_cast<vtkm::Scalar>(index)/100;
}

template<typename T, int N>
vtkm::Tuple<T,N> TestValue(vtkm::Id index, vtkm::Tuple<T,N>) {
  vtkm::Tuple<T,N> value;
  for (int i = 0; i < N; i++)
  {
    value[i] = TestValue(index, T()) + (i + 1);
  }
  return value;
}

std::string TestValue(vtkm::Id index, std::string) {
  std::stringstream stream;
  stream << index;
  return stream.str();
}

struct TypeListTagString : vtkm::ListTagBase<std::string> {  };

template<typename T>
struct UnusualPortal
{
  typedef T ValueType;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const { return TestValue(index, ValueType()); }

  typedef vtkm::cont::internal::IteratorFromArrayPortal<UnusualPortal<T> >
      IteratorType;

  VTKM_CONT_EXPORT
  IteratorType GetIteratorBegin() const {
    return IteratorType(*this);
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorEnd() const {
    return IteratorType(*this, this->GetNumberOfValues());
  }
};

template<typename T>
class ArrayHandleWithUnusualStorage
    : public vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagImplicit<UnusualPortal<T> > >
{
  typedef vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagImplicit<UnusualPortal<T> > >
      Superclass;
public:
  VTKM_CONT_EXPORT
  ArrayHandleWithUnusualStorage()
    : Superclass(typename Superclass::PortalConstControl()) {  }
};

struct StorageListTagUnusual :
    vtkm::ListTagBase2<
      ArrayHandleWithUnusualStorage<vtkm::Id>::StorageTag,
      ArrayHandleWithUnusualStorage<std::string>::StorageTag>
{  };

bool CheckCalled;

struct CheckFunctor
{
  template<typename T, typename Storage>
  void operator()(vtkm::cont::ArrayHandle<T, Storage> array) const {
    CheckCalled = true;

    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE,
                     "Unexpected array size.");

    typename vtkm::cont::ArrayHandle<T,Storage>::PortalConstControl portal =
        array.GetPortalConstControl();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      VTKM_TEST_ASSERT(portal.Get(index) == TestValue(index, T()),
                       "Got bad value in array. Perhaps a bad cast?");
    }
  }
};

template<typename T>
vtkm::cont::DynamicArrayHandle CreateDynamicArray(T)
{
  // Declared static to prevent going out of scope.
  static T buffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index, T());
  }

  return vtkm::cont::DynamicArrayHandle(
        vtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE));
}

template<typename T>
void TryDefaultType(T)
{
  CheckCalled = false;

  vtkm::cont::DynamicArrayHandle array = CreateDynamicArray(T());

  array.CastAndCall(CheckFunctor());

  VTKM_TEST_ASSERT(CheckCalled,
                   "The functor was never called (and apparently a bad value exception not thrown).");
}

struct TryBasicVTKmType
{
  template<typename T>
  void operator()(T) {
    CheckCalled = false;

    vtkm::cont::DynamicArrayHandle array = CreateDynamicArray(T());

    array.ResetTypeList(vtkm::TypeListTagAll()).CastAndCall(CheckFunctor());

    VTKM_TEST_ASSERT(CheckCalled,
                     "The functor was never called (and apparently a bad value exception not thrown).");
  }
};

void TryUnusualType()
{
  // A string is an unlikely type to be declared elsewhere in VTK-m.
  vtkm::cont::DynamicArrayHandle array = CreateDynamicArray(std::string());

  try
  {
    array.CastAndCall(CheckFunctor());
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized type.");
  }
  catch (vtkm::cont::ErrorControlBadValue)
  {
    std::cout << "  Caught exception for unrecognized type." << std::endl;
  }

  CheckCalled = false;
  array.ResetTypeList(TypeListTagString()).CastAndCall(CheckFunctor());
  VTKM_TEST_ASSERT(CheckCalled,
                   "The functor was never called (and apparently a bad value exception not thrown).");
  std::cout << "  Found type when type list was reset." << std:: endl;
}

void TryUnusualStorage()
{
  vtkm::cont::DynamicArrayHandle array =
      ArrayHandleWithUnusualStorage<vtkm::Id>();

  try
  {
    array.CastAndCall(CheckFunctor());
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized storage.");
  }
  catch (vtkm::cont::ErrorControlBadValue)
  {
    std::cout << "  Caught exception for unrecognized storage." << std::endl;
  }

  CheckCalled = false;
  array.ResetStorageList(StorageListTagUnusual()).CastAndCall(CheckFunctor());
  VTKM_TEST_ASSERT(CheckCalled,
                   "The functor was never called (and apparently a bad value exception not thrown).");
  std::cout << "  Found instance when storage list was reset." << std:: endl;
}

void TryUnusualTypeAndStorage()
{
  vtkm::cont::DynamicArrayHandle array =
      ArrayHandleWithUnusualStorage<std::string>();

  try
  {
    array.CastAndCall(CheckFunctor());
    VTKM_TEST_FAIL(
          "CastAndCall failed to error for unrecognized type/storage.");
  }
  catch (vtkm::cont::ErrorControlBadValue)
  {
    std::cout << "  Caught exception for unrecognized type/storage."
              << std::endl;
  }

  try
  {
    array.ResetTypeList(TypeListTagString()).CastAndCall(CheckFunctor());
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized storage.");
  }
  catch (vtkm::cont::ErrorControlBadValue)
  {
    std::cout << "  Caught exception for unrecognized storage." << std::endl;
  }

  try
  {
    array.ResetStorageList(StorageListTagUnusual()).
        CastAndCall(CheckFunctor());
    VTKM_TEST_FAIL("CastAndCall failed to error for unrecognized type.");
  }
  catch (vtkm::cont::ErrorControlBadValue)
  {
    std::cout << "  Caught exception for unrecognized type." << std::endl;
  }

  CheckCalled = false;
  array
      .ResetTypeList(TypeListTagString())
      .ResetStorageList(StorageListTagUnusual())
      .CastAndCall(CheckFunctor());
  VTKM_TEST_ASSERT(CheckCalled,
                   "The functor was never called (and apparently a bad value exception not thrown).");
  std::cout << "  Found instance when type and storage lists were reset." << std:: endl;

  CheckCalled = false;
  array
      .ResetStorageList(StorageListTagUnusual())
      .ResetTypeList(TypeListTagString())
      .CastAndCall(CheckFunctor());
  VTKM_TEST_ASSERT(CheckCalled,
                   "The functor was never called (and apparently a bad value exception not thrown).");
  std::cout << "  Found instance when storage and type lists were reset." << std:: endl;
}

void TestDynamicArrayHandle()
{
  std::cout << "Try common types with default type lists." << std::endl;
  TryDefaultType(vtkm::Id());
  TryDefaultType(vtkm::Scalar());
  TryDefaultType(vtkm::Vector3());

  std::cout << "Try all VTK-m types." << std::endl;
  vtkm::testing::Testing::TryAllTypes(TryBasicVTKmType());

  std::cout << "Try unusual type." << std::endl;
  TryUnusualType();

  std::cout << "Try unusual storage." << std::endl;
  TryUnusualStorage();

  std::cout << "Try unusual type in unusual storage." << std::endl;
  TryUnusualTypeAndStorage();
}

} // anonymous namespace

int UnitTestDynamicArrayHandle(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDynamicArrayHandle);
}
