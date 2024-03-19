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

#include <vtkm/Matrix.h>
#include <vtkm/NewtonsMethod.h>

#include <vtkm/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE NewtonsMethod
////
// A functor for the mathematical function f(x) = [dot(x,x),x[0]*x[1]]
struct FunctionFunctor
{
  template<typename T>
  VTKM_EXEC_CONT vtkm::Vec<T, 2> operator()(const vtkm::Vec<T, 2>& x) const
  {
    return vtkm::make_Vec(vtkm::Dot(x, x), x[0] * x[1]);
  }
};

// A functor for the Jacobian of the mathematical function
// f(x) = [dot(x,x),x[0]*x[1]], which is
//   | 2*x[0] 2*x[1] |
//   |   x[1]   x[0] |
struct JacobianFunctor
{
  template<typename T>
  VTKM_EXEC_CONT vtkm::Matrix<T, 2, 2> operator()(const vtkm::Vec<T, 2>& x) const
  {
    vtkm::Matrix<T, 2, 2> jacobian;
    jacobian(0, 0) = 2 * x[0];
    jacobian(0, 1) = 2 * x[1];
    jacobian(1, 0) = x[1];
    jacobian(1, 1) = x[0];

    return jacobian;
  }
};

VTKM_EXEC
void SolveNonlinear()
{
  // Use Newton's method to solve the nonlinear system of equations:
  //
  //    x^2 + y^2 = 2
  //    x*y = 1
  //
  // There are two possible solutions, which are (x=1,y=1) and (x=-1,y=-1).
  // The one found depends on the starting value.
  vtkm::NewtonsMethodResult<vtkm::Float32, 2> answer1 =
    vtkm::NewtonsMethod(JacobianFunctor(),
                        FunctionFunctor(),
                        vtkm::make_Vec(2.0f, 1.0f),
                        vtkm::make_Vec(1.0f, 0.0f));
  if (!answer1.Valid || !answer1.Converged)
  {
    // Failed to find solution
    //// PAUSE-EXAMPLE
    VTKM_TEST_FAIL("Could not find answer1");
    //// RESUME-EXAMPLE
  }
  // answer1.Solution is [1,1]

  vtkm::NewtonsMethodResult<vtkm::Float32, 2> answer2 =
    vtkm::NewtonsMethod(JacobianFunctor(),
                        FunctionFunctor(),
                        vtkm::make_Vec(2.0f, 1.0f),
                        vtkm::make_Vec(0.0f, -2.0f));
  if (!answer2.Valid || !answer2.Converged)
  {
    // Failed to find solution
    //// PAUSE-EXAMPLE
    VTKM_TEST_FAIL("Could not find answer2");
    //// RESUME-EXAMPLE
  }
  // answer2 is [-1,-1]
  //// PAUSE-EXAMPLE
  std::cout << answer1.Solution << " " << answer2.Solution << std::endl;

  VTKM_TEST_ASSERT(test_equal(answer1.Solution, vtkm::make_Vec(1, 1), 0.01),
                   "Bad answer 1.");
  VTKM_TEST_ASSERT(test_equal(answer2.Solution, vtkm::make_Vec(-1, -1), 0.01),
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
  return vtkm::testing::Testing::Run(Run, argc, argv);
}
