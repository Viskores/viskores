//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_NewtonsMethod_h
#define vtk_m_NewtonsMethod_h

#include <vtkm/Math.h>
#include <vtkm/Matrix.h>

namespace vtkm
{

template <typename ScalarType, vtkm::IdComponent Size>
struct NewtonsMethodResult
{
  bool Valid;
  bool Converged;
  vtkm::Vec<ScalarType, Size> Solution;
};

/// Uses Newton's method (a.k.a. Newton-Raphson method) to solve a nonlinear
/// system of equations. This function assumes that the number of variables
/// equals the number of equations. Newton's method operates on an iterative
/// evaluate and search. Evaluations are performed using the functors passed
/// into the NewtonsMethod. The first functor returns the NxN matrix of the
/// Jacobian at a given input point. The second functor returns the N tuple
/// that is the function evaluation at the given input point. The input point
/// that evaluates to the desired output, or the closest point found, is
/// returned.
///
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename ScalarType,
          vtkm::IdComponent Size,
          typename JacobianFunctor,
          typename FunctionFunctor>
VTKM_EXEC_CONT NewtonsMethodResult<ScalarType, Size> NewtonsMethod(
  JacobianFunctor jacobianEvaluator,
  FunctionFunctor functionEvaluator,
  vtkm::Vec<ScalarType, Size> desiredFunctionOutput,
  vtkm::Vec<ScalarType, Size> initialGuess = vtkm::Vec<ScalarType, Size>(ScalarType(0)),
  ScalarType convergeDifference = ScalarType(1e-3),
  vtkm::IdComponent maxIterations = 10)
{
  using VectorType = vtkm::Vec<ScalarType, Size>;
  using MatrixType = vtkm::Matrix<ScalarType, Size, Size>;

  VectorType x = initialGuess;

  bool valid = false;
  bool converged = false;
  for (vtkm::IdComponent iteration = 0; !converged && (iteration < maxIterations); iteration++)
  {
    // For Newton's method, we solve the linear system
    //
    // Jacobian x deltaX = currentFunctionOutput - desiredFunctionOutput
    //
    // The subtraction on the right side simply makes the target of the solve
    // at zero, which is what Newton's method solves for. The deltaX tells us
    // where to move to to solve for a linear system, which we assume will be
    // closer for our nonlinear system.

    MatrixType jacobian = jacobianEvaluator(x);
    VectorType currentFunctionOutput = functionEvaluator(x);

    VectorType deltaX =
      vtkm::SolveLinearSystem(jacobian, currentFunctionOutput - desiredFunctionOutput, valid);
    if (!valid)
    {
      break;
    }

    x = x - deltaX;

    converged = true;
    for (vtkm::IdComponent index = 0; index < Size; index++)
    {
      converged &= (vtkm::Abs(deltaX[index]) < convergeDifference);
    }
  }

  // Not checking whether converged.
  return { valid, converged, x };
}

} // namespace vtkm

#endif //vtk_m_NewtonsMethod_h
