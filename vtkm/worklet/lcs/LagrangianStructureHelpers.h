//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_lcs_LagrangianStructureHelpers_h
#define vtk_m_worklet_lcs_LagrangianStructureHelpers_h

#include <vtkm/Matrix.h>
#include <vtkm/Swap.h>
#include <vtkm/Types.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{

template <typename T>
VTKM_EXEC_CONT void ComputeLeftCauchyGreenTensor(vtkm::Matrix<T, 2, 2>& jacobian)
{
  vtkm::Vec<T, 2> j1 = vtkm::MatrixGetRow(jacobian, 0);
  vtkm::Vec<T, 2> j2 = vtkm::MatrixGetRow(jacobian, 1);

  // Left Cauchy Green Tensor is J*J^T
  // j1[0] j1[1] | j1[0] j2[0]
  // j2[0] j2[1] | j1[1] j2[1]

  T a = j1[0] * j1[0] + j1[1] * j1[1];
  T b = j1[0] * j2[0] + j1[1] * j2[1];

  T d = j2[0] * j2[0] + j2[1] * j2[1];

  vtkm::MatrixSetRow(jacobian, 0, vtkm::Vec<T, 2>(a, b));
  vtkm::MatrixSetRow(jacobian, 1, vtkm::Vec<T, 2>(b, d));
}

template <typename T>
VTKM_EXEC_CONT void ComputeLeftCauchyGreenTensor(vtkm::Matrix<T, 3, 3>& jacobian)
{
  vtkm::Vec<T, 3> j1 = vtkm::MatrixGetRow(jacobian, 0);
  vtkm::Vec<T, 3> j2 = vtkm::MatrixGetRow(jacobian, 1);
  vtkm::Vec<T, 3> j3 = vtkm::MatrixGetRow(jacobian, 2);

  // Left Cauchy Green Tensor is J*J^T
  // j1[0]  j1[1] j1[2] |  j1[0]  j2[0]  j3[0]
  // j2[0]  j2[1] j2[2] |  j1[1]  j2[1]  j3[1]
  // j3[0]  j3[1] j3[2] |  j1[2]  j2[2]  j3[2]

  T a = j1[0] * j1[0] + j1[1] * j1[1] + j1[2] * j1[2];
  T b = j1[0] * j2[0] + j1[1] * j2[1] + j1[2] * j2[2];
  T c = j1[0] * j3[0] + j1[1] * j3[1] + j1[2] * j3[2];

  T d = j2[0] * j2[0] + j2[1] * j2[1] + j2[2] * j2[2];
  T e = j2[0] * j3[0] + j2[1] * j3[1] + j2[2] * j3[2];

  T f = j3[0] * j3[0] + j3[1] * j3[1] + j3[2] * j3[2];

  vtkm::MatrixSetRow(jacobian, 0, vtkm::Vec<T, 3>(a, b, c));
  vtkm::MatrixSetRow(jacobian, 1, vtkm::Vec<T, 3>(b, d, e));
  vtkm::MatrixSetRow(jacobian, 2, vtkm::Vec<T, 3>(d, e, f));
}

template <typename T>
VTKM_EXEC_CONT void ComputeRightCauchyGreenTensor(vtkm::Matrix<T, 2, 2>& jacobian)
{
  vtkm::Vec<T, 2> j1 = vtkm::MatrixGetRow(jacobian, 0);
  vtkm::Vec<T, 2> j2 = vtkm::MatrixGetRow(jacobian, 1);

  // Right Cauchy Green Tensor is J^T*J
  // j1[0]  j2[0] | j1[0]  j1[1]
  // j1[1]  j2[1] | j2[0]  j2[1]

  T a = j1[0] * j1[0] + j2[0] * j2[0];
  T b = j1[0] * j1[1] + j2[0] * j2[1];

  T d = j1[1] * j1[1] + j2[1] * j2[1];

  j1 = vtkm::Vec<T, 2>(a, b);
  j2 = vtkm::Vec<T, 2>(b, d);
}

template <typename T>
VTKM_EXEC_CONT void ComputeRightCauchyGreenTensor(vtkm::Matrix<T, 3, 3>& jacobian)
{
  vtkm::Vec<T, 3> j1 = vtkm::MatrixGetRow(jacobian, 0);
  vtkm::Vec<T, 3> j2 = vtkm::MatrixGetRow(jacobian, 1);
  vtkm::Vec<T, 3> j3 = vtkm::MatrixGetRow(jacobian, 2);

  // Right Cauchy Green Tensor is J^T*J
  // j1[0]  j2[0]  j3[0] | j1[0]  j1[1] j1[2]
  // j1[1]  j2[1]  j3[1] | j2[0]  j2[1] j2[2]
  // j1[2]  j2[2]  j3[2] | j3[0]  j3[1] j3[2]

  T a = j1[0] * j1[0] + j2[0] * j2[0] + j3[0] * j3[0];
  T b = j1[0] * j1[1] + j2[0] * j2[1] + j3[0] * j3[1];
  T c = j1[0] * j1[2] + j2[0] * j2[2] + j3[0] * j3[2];

  T d = j1[1] * j1[1] + j2[1] * j2[1] + j3[1] * j3[1];
  T e = j1[1] * j1[2] + j2[1] * j2[2] + j3[1] * j3[2];

  T f = j1[2] * j1[2] + j2[2] * j2[2] + j3[2] * j3[2];

  j1 = vtkm::Vec<T, 3>(a, b, c);
  j2 = vtkm::Vec<T, 3>(b, d, e);
  j3 = vtkm::Vec<T, 3>(d, e, f);
}

template <typename T>
VTKM_EXEC_CONT void Jacobi(vtkm::Matrix<T, 2, 2> tensor, vtkm::Vec<T, 2>& eigen)
{
  vtkm::Vec<T, 2> j1 = vtkm::MatrixGetRow(tensor, 0);
  vtkm::Vec<T, 2> j2 = vtkm::MatrixGetRow(tensor, 1);

  // Assume a symetric matrix
  // a b
  // b c
  T a = j1[0];
  T b = j1[1];
  T c = j2[1];

  T trace = (a + c) / 2.0f;
  T det = a * c - b * b;
  T sqrtr = vtkm::Sqrt(trace * trace - det);

  // Arrange eigen values from largest to smallest.
  eigen[0] = trace + sqrtr;
  eigen[1] = trace - sqrtr;
}

template <typename T>
VTKM_EXEC_CONT void Jacobi(vtkm::Matrix<T, 3, 3> tensor, vtkm::Vec<T, 3>& eigen)
{
  vtkm::Vec<T, 3> j1 = vtkm::MatrixGetRow(tensor, 0);
  vtkm::Vec<T, 3> j2 = vtkm::MatrixGetRow(tensor, 1);
  vtkm::Vec<T, 3> j3 = vtkm::MatrixGetRow(tensor, 2);

  // Assume a symetric matrix
  // a b c
  // b d e
  // c e f
  T a = j1[0];
  T b = j1[1];
  T c = j1[2];
  T d = j2[1];
  T e = j2[2];
  T f = j3[2];

  T x = (a + d + f) / 3.0f; // trace

  a -= x;
  d -= x;
  f -= x;

  // Det / 2;
  T q = (a * d * f + b * e * c + c * b * e - c * d * c - e * e * a - f * b * b) / 2.0f;
  T r = (a * a + b * b + c * c + b * b + d * d + e * e + c * c + e * e + f * f) / 6.0f;

  T D = (r * r * r - q * q);
  T phi = 0.0f;

  if (D < vtkm::Epsilon<T>())
    phi = 0.0f;
  else
  {
    phi = vtkm::ATan(vtkm::Sqrt(D) / q) / 3.0f;

    if (phi < 0)
      phi += static_cast<T>(vtkm::Pi());
  }

  const T sqrt3 = vtkm::Sqrt(3.0f);
  const T sqrtr = vtkm::Sqrt(r);

  T sinphi = 0.0f, cosphi = 0.0f;
  sinphi = vtkm::Sin(phi);
  cosphi = vtkm::Cos(phi);

  T w0 = x + 2.0f * sqrtr * cosphi;
  T w1 = x - sqrtr * (cosphi - sqrt3 * sinphi);
  T w2 = x - sqrtr * (cosphi + sqrt3 * sinphi);

  // Arrange eigen values from largest to smallest.
  if (w1 > w0)
    vtkm::Swap(w0, w1);
  if (w2 > w0)
    vtkm::Swap(w0, w2);
  if (w2 > w1)
    vtkm::Swap(w1, w2);

  eigen[0] = w0;
  eigen[1] = w1;
  eigen[2] = w2;
}

} // namespace detail
} // namespace worklet
} // namespace vtkm
#endif //vtk_m_worklet_lcs_LagrangianStructureHelpers_h
