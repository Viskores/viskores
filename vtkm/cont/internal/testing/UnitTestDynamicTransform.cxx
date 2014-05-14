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

#include "vtkm/cont/internal/DynamicTransform.h"

#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/DynamicArrayHandle.h"
#include "vtkm/cont/DynamicPointCoordinates.h"

#include "vtkm/internal/FunctionInterface.h"

#include "vtkm/cont/testing/Testing.h"

namespace {

static int g_FunctionCalls;

#define TRY_TRANSFORM(expr) \
  g_FunctionCalls = 0; \
  expr; \
  VTKM_TEST_ASSERT(g_FunctionCalls == 1, "Functor not called correctly.")

struct TypeListTagString : vtkm::ListTagBase<std::string> { };

struct ScalarFunctor {
  void operator()(vtkm::Scalar) const {
    std::cout << "    In Scalar functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct ArrayHandleScalarFunctor {
  template<typename T>
  void operator()(const vtkm::cont::ArrayHandle<T> &) const {
    VTKM_TEST_FAIL("Called wrong form of functor operator.");
  }
  void operator()(const vtkm::cont::ArrayHandle<vtkm::Scalar> &) const {
    std::cout << "    In ArrayHandle<Scalar> functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct ArrayHandleStringFunctor {
  void operator()(const vtkm::cont::ArrayHandle<std::string> &) const {
    std::cout << "    In ArrayHandle<string> functor." << std::endl;
    g_FunctionCalls++;
  }
};

struct FunctionInterfaceFunctor {
  template<typename Signature>
  void operator()(const vtkm::internal::FunctionInterface<Signature> &) const {
    VTKM_TEST_FAIL("Called wrong form of functor operator.");
  }
  void operator()(
      const vtkm::internal::FunctionInterface<
        void(vtkm::cont::ArrayHandle<vtkm::Scalar>,
             vtkm::cont::ArrayHandle<vtkm::Scalar>,
             vtkm::cont::ArrayHandle<std::string>,
             vtkm::cont::ArrayHandleUniformPointCoordinates)> &) const {
    std::cout << "    In FunctionInterface<...> functor." << std::endl;
    g_FunctionCalls++;
  }
};

void TestBasicTransform()
{
  std::cout << "Testing basic transform." << std::endl;

  vtkm::cont::internal::DynamicTransform transform;

  std::cout << "  Trying with simple scalar." << std::endl;
  TRY_TRANSFORM(transform(vtkm::Scalar(5), ScalarFunctor()));

  std::cout << "  Trying with basic scalar array." << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Scalar> concreteArray;
  TRY_TRANSFORM(transform(concreteArray, ArrayHandleScalarFunctor()));

  std::cout << "  Trying scalar dynamic array." << std::endl;
  vtkm::cont::DynamicArrayHandle dynamicArray = concreteArray;
  TRY_TRANSFORM(transform(dynamicArray, ArrayHandleScalarFunctor()));

  std::cout << "  Trying with unusual (string) dynamic array." << std::endl;
  dynamicArray = vtkm::cont::ArrayHandle<std::string>();
  TRY_TRANSFORM(transform(dynamicArray.ResetTypeList(TypeListTagString()),
                          ArrayHandleStringFunctor()));
}

void TestFunctionTransform()
{
  std::cout << "Testing transforms in FunctionInterface." << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Scalar> scalarArray;
  vtkm::cont::ArrayHandle<std::string> stringArray;
  vtkm::cont::ArrayHandleUniformPointCoordinates pointCoordinatesArray;

  std::cout << "  Trying basic functor call w/o transform (make sure it works)."
            << std::endl;
  TRY_TRANSFORM(FunctionInterfaceFunctor()(
                  vtkm::internal::make_FunctionInterface<void>(
                    scalarArray,
                    scalarArray,
                    stringArray,
                    pointCoordinatesArray)));

  std::cout << "  Trying dynamic cast" << std::endl;
  TRY_TRANSFORM(
        vtkm::internal::make_FunctionInterface<void>(
          scalarArray,
          vtkm::cont::DynamicArrayHandle(scalarArray),
          vtkm::cont::DynamicArrayHandle(stringArray).ResetTypeList(TypeListTagString()),
          vtkm::cont::DynamicPointCoordinates(vtkm::cont::PointCoordinatesUniform()))
        .DynamicTransformCont(vtkm::cont::internal::DynamicTransform(),
                              FunctionInterfaceFunctor()));
}

void TestDynamicTransform()
{
  TestBasicTransform();
  TestFunctionTransform();
}

} // anonymous namespace

int UnitTestDynamicTransform(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDynamicTransform);
}
