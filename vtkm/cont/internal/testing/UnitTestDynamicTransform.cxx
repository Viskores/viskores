//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include "vtkm/cont/internal/DynamicTransform.h"

#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/DynamicArrayHandle.h"
#include "vtkm/cont/DynamicCellSet.h"

#include "vtkm/internal/FunctionInterface.h"

#include "vtkm/cont/testing/Testing.h"

namespace vtkm
{

// DynamicArrayHandle requires its value type to have a defined VecTraits
// class. One of the tests is to use an "unusual" array of std::string
// (which is pretty pointless but might tease out some assumptions).
// Make an implementation here. Because I am lazy, this is only a partial
// implementation.
template <>
struct VecTraits<std::string>
{
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 1;
  using HasMultipleComponents = vtkm::VecTraitsTagSingleComponent;
};

} // namespace vtkm

namespace
{

static int g_FunctionCalls;

#define TRY_TRANSFORM(expr)                                                                        \
  g_FunctionCalls = 0;                                                                             \
  expr;                                                                                            \
  VTKM_TEST_ASSERT(g_FunctionCalls == 1, "Functor not called correctly.")

struct TypeListTagString : vtkm::ListTagBase<std::string>
{
};

struct ScalarFunctor
{
  void operator()(vtkm::FloatDefault) const
  {
    std::cout << "    In Scalar functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct ArrayHandleScalarFunctor
{
  template <typename T>
  void operator()(const vtkm::cont::ArrayHandle<T>&) const
  {
    VTKM_TEST_FAIL("Called wrong form of functor operator.");
  }
  void operator()(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>&) const
  {
    std::cout << "    In ArrayHandle<Scalar> functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct ArrayHandleStringFunctor
{
  void operator()(const vtkm::cont::ArrayHandle<std::string>&) const
  {
    std::cout << "    In ArrayHandle<string> functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct CellSetStructuredFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    VTKM_TEST_FAIL("Called wrong form of functor operator.");
  }
  void operator()(const vtkm::cont::CellSetStructured<3>&) const
  {
    std::cout << "    In CellSetStructured<3> functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct FunctionInterfaceFunctor
{
  template <typename Signature>
  void operator()(const vtkm::internal::FunctionInterface<Signature>&) const
  {
    VTKM_TEST_FAIL("Called wrong form of functor operator.");
  }
  void operator()(
    const vtkm::internal::FunctionInterface<void(vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                                 vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                                 vtkm::cont::ArrayHandle<std::string>,
                                                 vtkm::cont::CellSetStructured<3>)>&) const
  {
    std::cout << "    In FunctionInterface<...> functor." << std::endl;
    g_FunctionCalls++;
  }
};

void TestBasicTransform()
{
  std::cout << "Testing basic transform." << std::endl;

  vtkm::cont::internal::DynamicTransform transform;
  vtkm::internal::IndexTag<1> indexTag;

  std::cout << "  Trying with simple scalar." << std::endl;
  TRY_TRANSFORM(transform(vtkm::FloatDefault(5), ScalarFunctor(), indexTag));

  std::cout << "  Trying with basic scalar array." << std::endl;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> concreteArray;
  TRY_TRANSFORM(transform(concreteArray, ArrayHandleScalarFunctor(), indexTag));

  std::cout << "  Trying scalar dynamic array." << std::endl;
  vtkm::cont::DynamicArrayHandle dynamicArray = concreteArray;
  TRY_TRANSFORM(transform(dynamicArray, ArrayHandleScalarFunctor(), indexTag));

  std::cout << "  Trying with unusual (string) dynamic array." << std::endl;
  dynamicArray = vtkm::cont::ArrayHandle<std::string>();
  TRY_TRANSFORM(transform(
    dynamicArray.ResetTypeList(TypeListTagString()), ArrayHandleStringFunctor(), indexTag));

  std::cout << "  Trying with structured cell set." << std::endl;
  vtkm::cont::CellSetStructured<3> concreteCellSet;
  TRY_TRANSFORM(transform(concreteCellSet, CellSetStructuredFunctor(), indexTag));

  std::cout << "  Trying with dynamic cell set." << std::endl;
  vtkm::cont::DynamicCellSet dynamicCellSet = concreteCellSet;
  TRY_TRANSFORM(transform(dynamicCellSet, CellSetStructuredFunctor(), indexTag));
}

void TestFunctionTransform()
{
  std::cout << "Testing transforms in FunctionInterface." << std::endl;

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scalarArray;
  vtkm::cont::ArrayHandle<std::string> stringArray;
  vtkm::cont::CellSetStructured<3> structuredCellSet;

  std::cout << "  Trying basic functor call w/o transform (make sure it works)." << std::endl;
  TRY_TRANSFORM(FunctionInterfaceFunctor()(vtkm::internal::make_FunctionInterface<void>(
    scalarArray, scalarArray, stringArray, structuredCellSet)));

  std::cout << "  Trying dynamic cast" << std::endl;
  TRY_TRANSFORM(
    vtkm::internal::make_FunctionInterface<void>(
      scalarArray,
      vtkm::cont::DynamicArrayHandle(scalarArray),
      vtkm::cont::DynamicArrayHandle(stringArray).ResetTypeList(TypeListTagString()),
      vtkm::cont::DynamicCellSet(structuredCellSet))
      .DynamicTransformCont(vtkm::cont::internal::DynamicTransform(), FunctionInterfaceFunctor()));
}

void TestDynamicTransform()
{
  TestBasicTransform();
  TestFunctionTransform();
}

} // anonymous namespace

int UnitTestDynamicTransform(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestDynamicTransform);
}
