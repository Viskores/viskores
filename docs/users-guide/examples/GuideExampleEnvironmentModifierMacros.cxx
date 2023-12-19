//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE EnvironmentModifierMacro
////
template<typename ValueType>
VTKM_EXEC_CONT ValueType Square(const ValueType& inValue)
{
  return inValue * inValue;
}
////
//// END-EXAMPLE EnvironmentModifierMacro
////

////
//// BEGIN-EXAMPLE SuppressExecWarnings
////
VTKM_SUPPRESS_EXEC_WARNINGS
template<typename Functor>
VTKM_EXEC_CONT void OverlyComplicatedForLoop(Functor& functor, vtkm::Id numInterations)
{
  for (vtkm::Id index = 0; index < numInterations; index++)
  {
    functor();
  }
}
////
//// END-EXAMPLE SuppressExecWarnings
////

struct TestFunctor
{
  vtkm::Id Count;

  VTKM_CONT
  TestFunctor()
    : Count(0)
  {
  }

  VTKM_CONT
  void operator()() { this->Count++; }
};

void Test()
{
  VTKM_TEST_ASSERT(Square(2) == 4, "Square function doesn't square.");

  TestFunctor functor;
  OverlyComplicatedForLoop(functor, 10);
  VTKM_TEST_ASSERT(functor.Count == 10, "Bad iterations.");
}

} // anonymous namespace

int GuideExampleEnvironmentModifierMacros(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(Test, argc, argv);
}
