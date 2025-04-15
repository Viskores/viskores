//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/Matrix.h>
#include <viskores/NewtonsMethod.h>

#include <viskores/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE NewtonsMethod
////
// A functor for the mathematical function f(x) = [dot(x,x),x[0]*x[1]]
struct FunctionFunctor
{
  template<typename T>
  VISKORES_EXEC_CONT viskores::Vec<T, 2> operator()(const viskores::Vec<T, 2>& x) const
  {
    return viskores::make_Vec(viskores::Dot(x, x), x[0] * x[1]);
  }
};

// A functor for the Jacobian of the mathematical function
// f(x) = [dot(x,x),x[0]*x[1]], which is
//   | 2*x[0] 2*x[1] |
//   |   x[1]   x[0] |
struct JacobianFunctor
{
  template<typename T>
  VISKORES_EXEC_CONT viskores::Matrix<T, 2, 2> operator()(
    const viskores::Vec<T, 2>& x) const
  {
    viskores::Matrix<T, 2, 2> jacobian;
    jacobian(0, 0) = 2 * x[0];
    jacobian(0, 1) = 2 * x[1];
    jacobian(1, 0) = x[1];
    jacobian(1, 1) = x[0];

    return jacobian;
  }
};

VISKORES_EXEC
void SolveNonlinear()
{
  // Use Newton's method to solve the nonlinear system of equations:
  //
  //    x^2 + y^2 = 2
  //    x*y = 1
  //
  // There are two possible solutions, which are (x=1,y=1) and (x=-1,y=-1).
  // The one found depends on the starting value.
  viskores::NewtonsMethodResult<viskores::Float32, 2> answer1 =
    viskores::NewtonsMethod(JacobianFunctor(),
                            FunctionFunctor(),
                            viskores::make_Vec(2.0f, 1.0f),
                            viskores::make_Vec(1.0f, 0.0f));
  if (!answer1.Valid || !answer1.Converged)
  {
    // Failed to find solution
    //// PAUSE-EXAMPLE
    VISKORES_TEST_FAIL("Could not find answer1");
    //// RESUME-EXAMPLE
  }
  // answer1.Solution is [1,1]

  viskores::NewtonsMethodResult<viskores::Float32, 2> answer2 =
    viskores::NewtonsMethod(JacobianFunctor(),
                            FunctionFunctor(),
                            viskores::make_Vec(2.0f, 1.0f),
                            viskores::make_Vec(0.0f, -2.0f));
  if (!answer2.Valid || !answer2.Converged)
  {
    // Failed to find solution
    //// PAUSE-EXAMPLE
    VISKORES_TEST_FAIL("Could not find answer2");
    //// RESUME-EXAMPLE
  }
  // answer2 is [-1,-1]
  //// PAUSE-EXAMPLE
  std::cout << answer1.Solution << " " << answer2.Solution << std::endl;

  VISKORES_TEST_ASSERT(test_equal(answer1.Solution, viskores::make_Vec(1, 1), 0.01),
                       "Bad answer 1.");
  VISKORES_TEST_ASSERT(test_equal(answer2.Solution, viskores::make_Vec(-1, -1), 0.01),
                       "Bad answer 2.");
  //// RESUME-EXAMPLE
}
////
//// END-EXAMPLE NewtonsMethod
////

void Run()
{
  SolveNonlinear();
}

} // anonymous namespace

int GuideExampleNewtonsMethod(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(Run, argc, argv);
}
