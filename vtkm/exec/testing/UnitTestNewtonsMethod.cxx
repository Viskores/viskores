//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/NewtonsMethod.h>

#include <vtkm/testing/Testing.h>

namespace {

// We will test Newton's method with the following three functions:
//
// f1(x,y,z) = x^2 + y^2 + z^2
// f2(x,y,z) = 2x - y + z
// f3(x,y,z) = x + y - z
//
// If we want the result of all three equations to be 1, then there are two
// valid solutions: (2/3, -1/3, -2/3) and (2/3, 2/3, 1/3).
template<typename T>
struct EvaluateFunctions
{
  typedef vtkm::Vec<T,3> Vector3;

  VTKM_EXEC_EXPORT
  Vector3 operator()(Vector3 x) const
  {
    Vector3 fx;
    fx[0] = x[0]*x[0] + x[1]*x[1] + x[2]*x[2];
    fx[1] = 2*x[0] - x[1] + x[2];
    fx[2] = x[0] + x[1] - x[2];
    return fx;
  }
};
template<typename T>
struct EvaluateJacobian
{
  typedef vtkm::Vec<T,3> Vector3;
  typedef vtkm::Matrix<T,3,3> Matrix3x3;

  VTKM_EXEC_EXPORT
  Matrix3x3 operator()(Vector3 x) const {
    Matrix3x3 jacobian;
    jacobian(0,0) = 2*x[0];  jacobian(0,1) = 2*x[1];  jacobian(0,2) = 2*x[2];
    jacobian(1,0) = 2;       jacobian(1,1) = -1;      jacobian(1,2) = 1;
    jacobian(2,0) = 1;       jacobian(2,1) = 1;       jacobian(2,2) = -1;
    return jacobian;
  }
};

template<typename T>
void TestNewtonsMethodTemplate()
{
  std::cout << "Testing Newton's Method." << std::endl;

  typedef vtkm::Vec<T,3> Vector3;

  Vector3 desiredOutput(1, 1, 1);
  Vector3 expected1(2.0f/3.0f, -1.0f/3.0f, -2.0f/3.0f);
  Vector3 expected2(2.0f/3.0f, 2.0f/3.0f, 1.0f/3.0f);

  Vector3 initialGuess;
  for (initialGuess[0] = 0.25f; initialGuess[0] <= 1; initialGuess[0] += 0.25f)
  {
    for (initialGuess[1] = 0.25f; initialGuess[1] <= 1; initialGuess[1] += 0.25f)
    {
      for (initialGuess[2] = 0.25f; initialGuess[2] <= 1; initialGuess[2] +=0.25f)
      {
        std::cout << "   " << initialGuess << std::endl;

        Vector3 solution =
            vtkm::exec::NewtonsMethod(EvaluateJacobian<T>(),
                                      EvaluateFunctions<T>(),
                                      desiredOutput,
                                      initialGuess,
                                      T(1e-6));

        VTKM_TEST_ASSERT(test_equal(solution, expected1)
                         || test_equal(solution, expected2),
                         "Newton's method did not converge to expected result.");
      }
    }
  }
}

void TestNewtonsMethod()
{
  std::cout << "*** Float32 *************************" << std::endl;
  TestNewtonsMethodTemplate<vtkm::Float32>();
  std::cout << "*** Float64 *************************" << std::endl;
  TestNewtonsMethodTemplate<vtkm::Float64>();
}

} // anonymous namespace

int UnitTestNewtonsMethod(int, char *[])
{
  return vtkm::testing::Testing::Run(TestNewtonsMethod);
}
