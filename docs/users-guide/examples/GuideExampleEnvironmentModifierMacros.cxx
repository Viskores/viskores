//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/Types.h>

#include <viskores/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE EnvironmentModifierMacro
////
template<typename ValueType>
VISKORES_EXEC_CONT ValueType Square(const ValueType& inValue)
{
  return inValue * inValue;
}
////
//// END-EXAMPLE EnvironmentModifierMacro
////

////
//// BEGIN-EXAMPLE SuppressExecWarnings
////
VISKORES_SUPPRESS_EXEC_WARNINGS
template<typename Functor>
VISKORES_EXEC_CONT void OverlyComplicatedForLoop(Functor& functor,
                                                 viskores::Id numInterations)
{
  for (viskores::Id index = 0; index < numInterations; index++)
  {
    functor();
  }
}
////
//// END-EXAMPLE SuppressExecWarnings
////

struct TestFunctor
{
  viskores::Id Count;

  VISKORES_CONT
  TestFunctor()
    : Count(0)
  {
  }

  VISKORES_CONT
  void operator()() { this->Count++; }
};

void Test()
{
  VISKORES_TEST_ASSERT(Square(2) == 4, "Square function doesn't square.");

  TestFunctor functor;
  OverlyComplicatedForLoop(functor, 10);
  VISKORES_TEST_ASSERT(functor.Count == 10, "Bad iterations.");
}

} // anonymous namespace

int GuideExampleEnvironmentModifierMacros(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(Test, argc, argv);
}
