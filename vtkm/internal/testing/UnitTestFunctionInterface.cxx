//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/testing/Testing.h>

#include <sstream>
#include <string>

namespace
{

using Type1 = vtkm::Id;
const Type1 Arg1 = 1234;

using Type2 = vtkm::Float64;
const Type2 Arg2 = 5678.125;

using Type3 = std::string;
const Type3 Arg3("Third argument");

using Type4 = vtkm::Vec3f_32;
const Type4 Arg4(1.2f, 3.4f, 5.6f);

using Type5 = vtkm::Id3;
const Type5 Arg5(4, 5, 6);

struct PointerTransform
{
  template <typename T, vtkm::IdComponent Index>
  struct ReturnType
  {
    using type = const T*;
  };

  template <typename T, typename IndexTag>
  const T* operator()(const T& x, IndexTag) const
  {
    return &x;
  }
};

void TryFunctionInterface5(
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3, Type4, Type5)> funcInterface)
{
  std::cout << "Checking 5 parameter function interface." << std::endl;
  VTKM_TEST_ASSERT(funcInterface.GetArity() == 5, "Got wrong number of parameters.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3, "Arg 3 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<4>() == Arg4, "Arg 4 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<5>() == Arg5, "Arg 5 incorrect.");


  std::cout << "Swizzling parameters with replace." << std::endl;
  funcInterface.Replace<1>(Arg5).Replace(Arg1, vtkm::internal::IndexTag<2>()).Replace<5>(Arg2);
}

void TestBasicFunctionInterface()
{
  std::cout << "Creating basic function interface." << std::endl;
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterface =
    vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  std::cout << "Checking parameters." << std::endl;
  VTKM_TEST_ASSERT(funcInterface.GetArity() == 3, "Got wrong number of parameters.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter(vtkm::internal::IndexTag<2>()) == Arg2,
                   "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3, "Arg 3 incorrect.");

  std::cout << "Checking invocation with argument modification." << std::endl;
  funcInterface.SetParameter<1>(Type1());
  funcInterface.SetParameter(Type2(), vtkm::internal::IndexTag<2>());
  funcInterface.SetParameter<3>(Type3());
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() != Arg1, "Arg 1 not cleared.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() != Arg2, "Arg 2 not cleared.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() != Arg3, "Arg 3 not cleared.");

  funcInterface.SetParameter(Arg2, vtkm::internal::IndexTag<2>());
  funcInterface.SetParameter<1>(Arg1);
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() != Arg3, "Arg 3 not cleared.");

  TryFunctionInterface5(vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3, Arg4, Arg5));
}

void TestAppend()
{
  std::cout << "Appending interface with return value." << std::endl;
  vtkm::internal::FunctionInterface<std::string(Type1, Type2)> funcInterface2ArgWRet =
    vtkm::internal::make_FunctionInterface<std::string>(Arg1, Arg2);

  vtkm::internal::FunctionInterface<std::string(Type1, Type2, Type3)> funcInterface3ArgWRet =
    funcInterface2ArgWRet.Append(Arg3);
  VTKM_TEST_ASSERT(funcInterface3ArgWRet.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface3ArgWRet.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface3ArgWRet.GetParameter<3>() == Arg3, "Arg 3 incorrect.");

  std::cout << "Appending another value." << std::endl;
  vtkm::internal::FunctionInterface<std::string(Type1, Type2, Type3, Type4)> funcInterface4ArgWRet =
    funcInterface3ArgWRet.Append(Arg4);
  VTKM_TEST_ASSERT(funcInterface4ArgWRet.GetParameter<4>() == Arg4, "Arg 4 incorrect.");

  std::cout << "Checking double append." << std::endl;
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterface3 =
    vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);
  TryFunctionInterface5(funcInterface3.Append(Arg4).Append(Arg5));
}

void TestStaticTransform()
{
  std::cout << "Trying static transform." << std::endl;
  using OriginalType = vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)>;
  OriginalType funcInterface = vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  std::cout << "Transform to pointer type." << std::endl;
  auto funcInterfaceTransform1 = funcInterface.StaticTransformCont(PointerTransform());

  using P1 = typename std::decay<decltype(funcInterfaceTransform1.GetParameter<1>())>::type;
  using P2 = typename std::decay<decltype(funcInterfaceTransform1.GetParameter<2>())>::type;
  using P3 = typename std::decay<decltype(funcInterfaceTransform1.GetParameter<3>())>::type;

  VTKM_STATIC_ASSERT((std::is_same<const Type1*, P1>::value));
  VTKM_STATIC_ASSERT((std::is_same<const Type2*, P2>::value));
  VTKM_STATIC_ASSERT((std::is_same<const Type3*, P3>::value));
}

void TestFunctionInterface()
{
  TestBasicFunctionInterface();
  TestAppend();
  TestStaticTransform();
}

} // anonymous namespace

int UnitTestFunctionInterface(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestFunctionInterface, argc, argv);
}
