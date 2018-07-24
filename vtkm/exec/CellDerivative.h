//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_Derivative_h
#define vtk_m_exec_Derivative_h

#include <vtkm/Assert.h>
#include <vtkm/BaseComponent.h>
#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/Jacobian.h>

namespace vtkm
{
namespace exec
{

// The derivative for a 2D polygon in 3D space is underdetermined since there
// is no information in the direction perpendicular to the polygon. To compute
// derivatives for general polygons, we build a 2D space for the polygon's
// plane and solve the derivative there.

namespace
{
#define VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D(pointIndex, weight0, weight1, weight2)                 \
  parametricDerivative[0] += field[pointIndex] * (weight0);                                        \
  parametricDerivative[1] += field[pointIndex] * (weight1);                                        \
  parametricDerivative[2] += field[pointIndex] * (weight2)

// Find the derivative of a field in parametric space. That is, find the
// vector [ds/du, ds/dv, ds/dw].
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> ParametricDerivative(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagHexahedron)
{
  using FieldType = typename FieldVecType::ComponentType;
  using GradientType = vtkm::Vec<FieldType, 3>;

  GradientType pc(pcoords);
  GradientType rc = GradientType(FieldType(1)) - pc;

  GradientType parametricDerivative(FieldType(0));
  VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);
  return parametricDerivative;
}

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> ParametricDerivative(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagWedge)
{
  using FieldType = typename FieldVecType::ComponentType;
  using GradientType = vtkm::Vec<FieldType, 3>;

  GradientType pc(pcoords);
  GradientType rc = GradientType(FieldType(1)) - pc;

  GradientType parametricDerivative(FieldType(0));
  VTKM_DERIVATIVE_WEIGHTS_WEDGE(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);

  return parametricDerivative;
}

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> ParametricDerivative(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPyramid)
{
  using FieldType = typename FieldVecType::ComponentType;
  using GradientType = vtkm::Vec<FieldType, 3>;

  GradientType pc(pcoords);
  GradientType rc = GradientType(FieldType(1)) - pc;

  GradientType parametricDerivative(FieldType(0));
  VTKM_DERIVATIVE_WEIGHTS_PYRAMID(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);

  return parametricDerivative;
}

#undef VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D

#define VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D(pointIndex, weight0, weight1)                          \
  parametricDerivative[0] += field[pointIndex] * (weight0);                                        \
  parametricDerivative[1] += field[pointIndex] * (weight1)

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 2> ParametricDerivative(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagQuad)
{
  using FieldType = typename FieldVecType::ComponentType;
  using GradientType = vtkm::Vec<FieldType, 2>;

  GradientType pc(static_cast<FieldType>(pcoords[0]), static_cast<FieldType>(pcoords[1]));
  GradientType rc = GradientType(FieldType(1)) - pc;

  GradientType parametricDerivative(FieldType(0));
  VTKM_DERIVATIVE_WEIGHTS_QUAD(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D);

  return parametricDerivative;
}

#if 0
// This code doesn't work, so I'm bailing on it. Instead, I'm just grabbing a
// triangle and finding the derivative of that. If you can do better, please
// implement it.
template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC
vtkm::Vec<typename FieldVecType::ComponentType,2>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagPolygon)
{
  using FieldType = typename FieldVecType::ComponentType;
  using GradientType = vtkm::Vec<FieldType,2>;

  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  FieldType deltaAngle = static_cast<FieldType>(2*vtkm::Pi()/numPoints);

  GradientType pc(pcoords[0], pcoords[1]);

  GradientType parametricDerivative(0);
  for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    FieldType angle = pointIndex*deltaAngle;
    vtkm::Vec<FieldType,2> nodePCoords(0.5f*(vtkm::Cos(angle)+1),
                                       0.5f*(vtkm::Sin(angle)+1));

    // This is the vector pointing from the user provided parametric coordinate
    // to the node at pointIndex in parametric space.
    vtkm::Vec<FieldType,2> pvec = nodePCoords - pc;

    // The weight (the derivative of the interpolation factor) happens to be
    // pvec scaled by the cube root of pvec's magnitude.
    FieldType magSqr = vtkm::MagnitudeSquared(pvec);
    FieldType invMag = vtkm::RSqrt(magSqr);
    FieldType scale = invMag*invMag*invMag;
    vtkm::Vec<FieldType,2> weight = scale*pvec;

    parametricDerivative[0] += field[pointIndex] * weight[0];
    parametricDerivative[1] += field[pointIndex] * weight[1];
  }

  return parametricDerivative;
}
#endif

#undef VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D

} // namespace unnamed

namespace detail
{

template <typename FieldType,
          int NumPoints,
          typename WorldCoordType,
          typename ParametricCoordType,
          typename CellShapeTag>
VTKM_EXEC vtkm::Vec<FieldType, 3> CellDerivativeFor3DCell(
  const vtkm::Vec<FieldType, NumPoints>& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag tag)
{
  using GradientType = vtkm::Vec<FieldType, 3>;

  // For reasons that should become apparent in a moment, we actually want
  // the transpose of the Jacobian.
  vtkm::Matrix<FieldType, 3, 3> jacobianTranspose;
  vtkm::exec::JacobianFor3DCell(wCoords, pcoords, jacobianTranspose, tag);
  jacobianTranspose = vtkm::MatrixTranspose(jacobianTranspose);

  GradientType parametricDerivative = ParametricDerivative(field, pcoords, tag);

  // If we write out the matrices below, it should become clear that the
  // Jacobian transpose times the field derivative in world space equals
  // the field derivative in parametric space.
  //
  //   |                     |  |       |     |       |
  //   | dx/du  dy/du  dz/du |  | ds/dx |     | ds/du |
  //   |                     |  |       |     |       |
  //   | dx/dv  dy/dv  dz/dv |  | ds/dy |  =  | ds/dv |
  //   |                     |  |       |     |       |
  //   | dx/dw  dy/dw  dz/dw |  | ds/dz |     | ds/dw |
  //   |                     |  |       |     |       |
  //
  // Now we just need to solve this linear system to find the derivative in
  // world space.

  bool valid; // Ignored.
  return vtkm::SolveLinearSystem(jacobianTranspose, parametricDerivative, valid);
}

template <typename T,
          int N,
          int NumPoints,
          typename WorldCoordType,
          typename ParametricCoordType,
          typename CellShapeTag>
VTKM_EXEC vtkm::Vec<vtkm::Vec<T, N>, 3> CellDerivativeFor3DCell(
  const vtkm::Vec<vtkm::Vec<T, N>, NumPoints>& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag tag)
{
  //We have been given a vector field so we need to solve for each
  //component of the vector. For explanation of the logic used see the
  //scalar version of CellDerivativeFor3DCell.
  vtkm::Matrix<T, 3, 3> perComponentJacobianTranspose;
  vtkm::exec::JacobianFor3DCell(wCoords, pcoords, perComponentJacobianTranspose, tag);
  perComponentJacobianTranspose = vtkm::MatrixTranspose(perComponentJacobianTranspose);

  bool valid; // Ignored.
  vtkm::Vec<vtkm::Vec<T, N>, 3> result(vtkm::Vec<T, N>(0.0f));
  vtkm::Vec<T, NumPoints> perPoint;
  for (vtkm::IdComponent i = 0; i < N; ++i)
  {
    for (vtkm::IdComponent c = 0; c < NumPoints; ++c)
    {
      perPoint[c] = field[c][i];
    }
    vtkm::Vec<T, 3> p = ParametricDerivative(perPoint, pcoords, tag);
    vtkm::Vec<T, 3> grad = vtkm::SolveLinearSystem(perComponentJacobianTranspose, p, valid);

    result[0][i] += grad[0];
    result[1][i] += grad[1];
    result[2][i] += grad[2];
  }
  return result;
}

template <typename FieldType, typename LUType, typename ParametricCoordType, typename CellShapeTag>
VTKM_EXEC vtkm::Vec<FieldType, 3> CellDerivativeFor2DCellFinish(
  const vtkm::Vec<FieldType, 4>& field,
  const vtkm::Matrix<LUType, 2, 2>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 2>& LUPermutation,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  const vtkm::exec::internal::Space2D<LUType>& space,
  CellShapeTag,
  vtkm::TypeTraitsScalarTag)
{
  // Finish solving linear equation. See CellDerivativeFor2DCell implementation
  // for more detail.
  vtkm::Vec<FieldType, 2> parametricDerivative =
    ParametricDerivative(field, pcoords, CellShapeTag());

  vtkm::Vec<FieldType, 2> gradient2D =
    vtkm::detail::MatrixLUPSolve(LUFactorization, LUPermutation, parametricDerivative);

  return space.ConvertVecFromSpace(gradient2D);
}

template <typename FieldType, typename LUType, typename ParametricCoordType, typename CellShapeTag>
VTKM_EXEC vtkm::Vec<FieldType, 3> CellDerivativeFor2DCellFinish(
  const vtkm::Vec<FieldType, 4>& field,
  const vtkm::Matrix<LUType, 2, 2>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 2>& LUPermutation,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  const vtkm::exec::internal::Space2D<LUType>& space,
  CellShapeTag,
  vtkm::TypeTraitsVectorTag)
{
  using FieldTraits = vtkm::VecTraits<FieldType>;
  using FieldComponentType = typename FieldTraits::ComponentType;

  vtkm::Vec<FieldType, 3> gradient(vtkm::TypeTraits<FieldType>::ZeroInitialization());

  for (vtkm::IdComponent fieldComponent = 0;
       fieldComponent < FieldTraits::GetNumberOfComponents(field[0]);
       fieldComponent++)
  {
    vtkm::Vec<FieldComponentType, 4> subField(FieldTraits::GetComponent(field[0], fieldComponent),
                                              FieldTraits::GetComponent(field[1], fieldComponent),
                                              FieldTraits::GetComponent(field[2], fieldComponent),
                                              FieldTraits::GetComponent(field[3], fieldComponent));

    vtkm::Vec<FieldComponentType, 3> subGradient = CellDerivativeFor2DCellFinish(
      subField,
      LUFactorization,
      LUPermutation,
      pcoords,
      space,
      CellShapeTag(),
      typename vtkm::TypeTraits<FieldComponentType>::DimensionalityTag());

    FieldTraits::SetComponent(gradient[0], fieldComponent, subGradient[0]);
    FieldTraits::SetComponent(gradient[1], fieldComponent, subGradient[1]);
    FieldTraits::SetComponent(gradient[2], fieldComponent, subGradient[2]);
  }

  return gradient;
}

template <typename FieldType, typename LUType, typename ParametricCoordType, typename CellShapeTag>
VTKM_EXEC vtkm::Vec<FieldType, 3> CellDerivativeFor2DCellFinish(
  const vtkm::Vec<FieldType, 4>& field,
  const vtkm::Matrix<LUType, 2, 2>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 2>& LUPermutation,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  const vtkm::exec::internal::Space2D<LUType>& space,
  CellShapeTag,
  vtkm::TypeTraitsMatrixTag)
{
  return CellDerivativeFor2DCellFinish(field,
                                       LUFactorization,
                                       LUPermutation,
                                       pcoords,
                                       space,
                                       CellShapeTag(),
                                       vtkm::TypeTraitsVectorTag());
}

template <typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType,
          typename CellShapeTag>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivativeFor2DCell(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag)
{
  using FieldType = typename FieldVecType::ComponentType;
  using BaseFieldType = typename BaseComponent<FieldType>::Type;

  // We have an underdetermined system in 3D, so create a 2D space in the
  // plane that the polygon sits.
  vtkm::exec::internal::Space2D<BaseFieldType> space(
    wCoords[0], wCoords[1], wCoords[wCoords.GetNumberOfComponents() - 1]);

  // For reasons that should become apparent in a moment, we actually want
  // the transpose of the Jacobian.
  vtkm::Matrix<BaseFieldType, 2, 2> jacobianTranspose;
  vtkm::exec::JacobianFor2DCell(wCoords, pcoords, space, jacobianTranspose, CellShapeTag());
  jacobianTranspose = vtkm::MatrixTranspose(jacobianTranspose);

  // Find the derivative of the field in parametric coordinate space. That is,
  // find the vector [ds/du, ds/dv].
  // Commented because this is actually done in CellDerivativeFor2DCellFinish
  // to handle vector fields.
  //  vtkm::Vec<BaseFieldType,2> parametricDerivative =
  //      ParametricDerivative(field,pcoords,CellShapeTag());

  // If we write out the matrices below, it should become clear that the
  // Jacobian transpose times the field derivative in world space equals
  // the field derivative in parametric space.
  //
  //   |                |  |        |     |       |
  //   | db0/du  db1/du |  | ds/db0 |     | ds/du |
  //   |                |  |        |  =  |       |
  //   | db0/dv  db1/dv |  | ds/db1 |     | ds/dv |
  //   |                |  |        |     |       |
  //
  // Now we just need to solve this linear system to find the derivative in
  // world space.

  bool valid; // Ignored.
  // If you look at the implementation of vtkm::SolveLinearSystem, you will see
  // that it is done in two parts. First, it does an LU factorization and then
  // uses that result to complete the solve. The factorization part talkes the
  // longest amount of time, and if we are performing the gradient on a vector
  // field, the factorization can be reused for each component of the vector
  // field. Thus, we are going to call the internals of SolveLinearSystem
  // ourselves to do the factorization and then apply it to all components.
  vtkm::Vec<vtkm::IdComponent, 2> permutation;
  BaseFieldType inversionParity; // Unused
  vtkm::detail::MatrixLUPFactor(jacobianTranspose, permutation, inversionParity, valid);
  // MatrixLUPFactor does in place factorization. jacobianTranspose is now the
  // LU factorization.
  return CellDerivativeFor2DCellFinish(field,
                                       jacobianTranspose,
                                       permutation,
                                       pcoords,
                                       space,
                                       CellShapeTag(),
                                       typename vtkm::TypeTraits<FieldType>::DimensionalityTag());
}

} // namespace detail

//-----------------------------------------------------------------------------
/// \brief Take the derivative (get the gradient) of a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, finds the derivative with respect to each
/// coordinate (i.e. the gradient) at that point. The derivative is not always
/// constant in some "linear" cells.
///
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& pointFieldValues,
  const WorldCoordType& worldCoordinateValues,
  const vtkm::Vec<ParametricCoordType, 3>& parametricCoords,
  vtkm::CellShapeTagGeneric shape,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::Vec<typename FieldVecType::ComponentType, 3> result;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      result = CellDerivative(
        pointFieldValues, worldCoordinateValues, parametricCoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Unknown cell shape sent to derivative.");
      return vtkm::Vec<typename FieldVecType::ComponentType, 3>();
  }
  return result;
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType&,
  const WorldCoordType&,
  const vtkm::Vec<ParametricCoordType, 3>&,
  vtkm::CellShapeTagEmpty,
  const vtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to take derivative in empty cell.");
  return vtkm::Vec<typename FieldVecType::ComponentType, 3>();
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>&,
  vtkm::CellShapeTagVertex,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  (void)field;
  (void)wCoords;
  VTKM_ASSERT(field.GetNumberOfComponents() == 1);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 1);

  using GradientType = vtkm::Vec<typename FieldVecType::ComponentType, 3>;
  return vtkm::TypeTraits<GradientType>::ZeroInitialization();
}

//-----------------------------------------------------------------------------
namespace detail
{

template <typename FieldType, typename WorldCoordType>
VTKM_EXEC vtkm::Vec<FieldType, 3> CellDerivativeLineImpl(
  const FieldType& deltaField,
  const WorldCoordType& vec, // direction of line
  const typename WorldCoordType::ComponentType& vecMagSqr,
  vtkm::TypeTraitsScalarTag)
{
  using GradientType = vtkm::Vec<FieldType, 3>;

  // The derivative of a line is in the direction of the line. Its length is
  // equal to the difference of the scalar field divided by the length of the
  // line segment. Thus, the derivative is characterized by
  // (deltaField*vec)/mag(vec)^2.

  return (deltaField / static_cast<FieldType>(vecMagSqr)) * GradientType(vec);
}

template <typename FieldType, typename WorldCoordType, typename VectorTypeTraitsTag>
VTKM_EXEC vtkm::Vec<FieldType, 3> CellDerivativeLineImpl(
  const FieldType& deltaField,
  const WorldCoordType& vec, // direction of line
  const typename WorldCoordType::ComponentType& vecMagSqr,
  VectorTypeTraitsTag)
{
  using FieldTraits = vtkm::VecTraits<FieldType>;
  using FieldComponentType = typename FieldTraits::ComponentType;
  using GradientType = vtkm::Vec<FieldType, 3>;

  GradientType gradient(vtkm::TypeTraits<FieldType>::ZeroInitialization());
  for (vtkm::IdComponent fieldComponent = 0;
       fieldComponent < FieldTraits::GetNumberOfComponents(deltaField);
       fieldComponent++)
  {
    using SubGradientType = vtkm::Vec<FieldComponentType, 3>;
    SubGradientType subGradient =
      CellDerivativeLineImpl(FieldTraits::GetComponent(deltaField, fieldComponent),
                             vec,
                             vecMagSqr,
                             typename vtkm::TypeTraits<FieldComponentType>::DimensionalityTag());
    FieldTraits::SetComponent(gradient[0], fieldComponent, subGradient[0]);
    FieldTraits::SetComponent(gradient[1], fieldComponent, subGradient[1]);
    FieldTraits::SetComponent(gradient[2], fieldComponent, subGradient[2]);
  }

  return gradient;
}

} // namespace detail

template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& vtkmNotUsed(pcoords),
  vtkm::CellShapeTagLine,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 2);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 2);

  using FieldType = typename FieldVecType::ComponentType;
  using BaseComponentType = typename BaseComponent<FieldType>::Type;

  FieldType deltaField(field[1] - field[0]);
  vtkm::Vec<BaseComponentType, 3> vec(wCoords[1] - wCoords[0]);

  return detail::CellDerivativeLineImpl(deltaField,
                                        vec,
                                        vtkm::MagnitudeSquared(vec),
                                        typename vtkm::TypeTraits<FieldType>::DimensionalityTag());
}

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const vtkm::VecAxisAlignedPointCoordinates<1>& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& vtkmNotUsed(pcoords),
  vtkm::CellShapeTagLine,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 2);

  using T = typename FieldVecType::ComponentType;

  return vtkm::Vec<T, 3>(
    static_cast<T>((field[1] - field[0]) / wCoords.GetSpacing()[0]), T(0), T(0));
}

//-----------------------------------------------------------------------------
namespace detail
{

template <typename ValueType, typename LUType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TriangleDerivativeFinish(
  const vtkm::Vec<ValueType, 3>& field,
  const vtkm::Matrix<LUType, 3, 3>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 3>& LUPermutation,
  vtkm::TypeTraitsScalarTag)
{
  // Finish solving linear equation. See TriangleDerivative implementation
  // for more detail.
  vtkm::Vec<LUType, 3> b(field[1] - field[0], field[2] - field[0], 0);

  return vtkm::detail::MatrixLUPSolve(LUFactorization, LUPermutation, b);
}

template <typename ValueType, typename LUType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TriangleDerivativeFinish(
  const vtkm::Vec<ValueType, 3>& field,
  const vtkm::Matrix<LUType, 3, 3>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 3>& LUPermutation,
  vtkm::TypeTraitsVectorTag)
{
  using FieldTraits = vtkm::VecTraits<ValueType>;
  using FieldComponentType = typename FieldTraits::ComponentType;

  vtkm::Vec<ValueType, 3> gradient(vtkm::TypeTraits<ValueType>::ZeroInitialization());

  for (vtkm::IdComponent fieldComponent = 0;
       fieldComponent < FieldTraits::GetNumberOfComponents(field[0]);
       fieldComponent++)
  {
    vtkm::Vec<FieldComponentType, 3> subField(FieldTraits::GetComponent(field[0], fieldComponent),
                                              FieldTraits::GetComponent(field[1], fieldComponent),
                                              FieldTraits::GetComponent(field[2], fieldComponent));

    vtkm::Vec<FieldComponentType, 3> subGradient =
      TriangleDerivativeFinish(subField,
                               LUFactorization,
                               LUPermutation,
                               typename vtkm::TypeTraits<FieldComponentType>::DimensionalityTag());

    FieldTraits::SetComponent(gradient[0], fieldComponent, subGradient[0]);
    FieldTraits::SetComponent(gradient[1], fieldComponent, subGradient[1]);
    FieldTraits::SetComponent(gradient[2], fieldComponent, subGradient[2]);
  }

  return gradient;
}

template <typename ValueType, typename LUType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TriangleDerivativeFinish(
  const vtkm::Vec<ValueType, 3>& field,
  const vtkm::Matrix<LUType, 3, 3>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 3>& LUPermutation,
  vtkm::TypeTraitsMatrixTag)
{
  return TriangleDerivativeFinish(
    field, LUFactorization, LUPermutation, vtkm::TypeTraitsVectorTag());
}

template <typename ValueType, typename WCoordType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TriangleDerivative(const vtkm::Vec<ValueType, 3>& field,
                                                     const vtkm::Vec<WCoordType, 3>& wCoords)
{
  using BaseComponentType = typename BaseComponent<ValueType>::Type;

  // The scalar values of the three points in a triangle completely specify a
  // linear field (with constant gradient) assuming the field is constant in
  // the normal direction to the triangle. The field, defined by the 3-vector
  // gradient g and scalar value s_origin, can be found with this set of 4
  // equations and 4 unknowns.
  //
  // Dot(p0, g) + s_origin = s0
  // Dot(p1, g) + s_origin = s1
  // Dot(p2, g) + s_origin = s2
  // Dot(n, g)             = 0
  //
  // Where the p's are point coordinates and n is the normal vector. But we
  // don't really care about s_origin. We just want to find the gradient g.
  // With some simple elimination we, we can get rid of s_origin and be left
  // with 3 equations and 3 unknowns.
  //
  // Dot(p1-p0, g) = s1 - s0
  // Dot(p2-p0, g) = s2 - s0
  // Dot(n, g)     = 0
  //
  // We'll solve this by putting this in matrix form Ax = b where the rows of A
  // are the differences in points and normal, b has the scalar differences,
  // and x is really the gradient g.

  vtkm::Vec<BaseComponentType, 3> v0 = wCoords[1] - wCoords[0];
  vtkm::Vec<BaseComponentType, 3> v1 = wCoords[2] - wCoords[0];
  vtkm::Vec<BaseComponentType, 3> n = vtkm::Cross(v0, v1);

  vtkm::Matrix<BaseComponentType, 3, 3> A;
  vtkm::MatrixSetRow(A, 0, v0);
  vtkm::MatrixSetRow(A, 1, v1);
  vtkm::MatrixSetRow(A, 2, n);

  // If the triangle is degenerate, then valid will be false. For now we are
  // ignoring it. We could detect it if we determine we need to although I have
  // seen singular matrices missed due to floating point error.
  //
  bool valid;

  // If you look at the implementation of vtkm::SolveLinearSystem, you will see
  // that it is done in two parts. First, it does an LU factorization and then
  // uses that result to complete the solve. The factorization part talkes the
  // longest amount of time, and if we are performing the gradient on a vector
  // field, the factorization can be reused for each component of the vector
  // field. Thus, we are going to call the internals of SolveLinearSystem
  // ourselves to do the factorization and then apply it to all components.
  vtkm::Vec<vtkm::IdComponent, 3> permutation;
  BaseComponentType inversionParity; // Unused
  vtkm::detail::MatrixLUPFactor(A, permutation, inversionParity, valid);
  // MatrixLUPFactor does in place factorization. A is now the LU factorization.
  return TriangleDerivativeFinish(
    field, A, permutation, typename vtkm::TypeTraits<ValueType>::DimensionalityTag());
}

} // namespace detail

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& inputField,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& vtkmNotUsed(pcoords),
  vtkm::CellShapeTagTriangle,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(inputField.GetNumberOfComponents() == 3);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 3);

  using ValueType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;
  vtkm::Vec<ValueType, 3> field;
  inputField.CopyInto(field);
  vtkm::Vec<WCoordType, 3> wpoints;
  wCoords.CopyInto(wpoints);
  return detail::TriangleDerivative(field, wpoints);
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VTKM_EXEC void PolygonComputeIndices(const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                     vtkm::IdComponent numPoints,
                                     vtkm::IdComponent& firstPointIndex,
                                     vtkm::IdComponent& secondPointIndex)
{
  ParametricCoordType angle;
  if ((vtkm::Abs(pcoords[0] - 0.5f) < 4 * vtkm::Epsilon<ParametricCoordType>()) &&
      (vtkm::Abs(pcoords[1] - 0.5f) < 4 * vtkm::Epsilon<ParametricCoordType>()))
  {
    angle = 0;
  }
  else
  {
    angle = vtkm::ATan2(pcoords[1] - 0.5f, pcoords[0] - 0.5f);
    if (angle < 0)
    {
      angle += static_cast<ParametricCoordType>(2 * vtkm::Pi());
    }
  }

  const ParametricCoordType deltaAngle =
    static_cast<ParametricCoordType>(2 * vtkm::Pi() / numPoints);
  firstPointIndex = static_cast<vtkm::IdComponent>(vtkm::Floor(angle / deltaAngle));
  secondPointIndex = firstPointIndex + 1;
  if (secondPointIndex == numPoints)
  {
    secondPointIndex = 0;
  }
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> PolygonDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  vtkm::IdComponent numPoints,
  vtkm::IdComponent firstPointIndex,
  vtkm::IdComponent secondPointIndex)
{
  // If we are here, then there are 5 or more points on this polygon.

  // Arrange the points such that they are on the circle circumscribed in the
  // unit square from 0 to 1. That is, the point are on the circle centered at
  // coordinate 0.5,0.5 with radius 0.5. The polygon is divided into regions
  // defined by they triangle fan formed by the points around the center. This
  // is C0 continuous but not necessarily C1 continuous. It is also possible to
  // have a non 1 to 1 mapping between parametric coordinates world coordinates
  // if the polygon is not planar or convex.

  using FieldType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;

  // Find the interpolation for the center point.
  FieldType fieldCenter = field[0];
  WCoordType wcoordCenter = wCoords[0];
  for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
  {
    fieldCenter = fieldCenter + field[pointIndex];
    wcoordCenter = wcoordCenter + wCoords[pointIndex];
  }
  fieldCenter = fieldCenter * FieldType(1.0f / static_cast<float>(numPoints));
  wcoordCenter = wcoordCenter * WCoordType(1.0f / static_cast<float>(numPoints));

  // Set up parameters for triangle that pcoords is in
  vtkm::Vec<FieldType, 3> triangleField(
    fieldCenter, field[firstPointIndex], field[secondPointIndex]);

  vtkm::Vec<WCoordType, 3> triangleWCoords(
    wcoordCenter, wCoords[firstPointIndex], wCoords[secondPointIndex]);

  // Now use the triangle derivative. pcoords is actually invalid for the
  // triangle, but that does not matter as the derivative for a triangle does
  // not depend on it.
  return detail::TriangleDerivative(triangleField, triangleWCoords);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPolygon,
  const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSERT(field.GetNumberOfComponents() == wCoords.GetNumberOfComponents());

  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  VTKM_ASSERT(numPoints > 0);

  switch (field.GetNumberOfComponents())
  {
    case 1:
      return CellDerivative(field, wCoords, pcoords, vtkm::CellShapeTagVertex(), worklet);
    case 2:
      return CellDerivative(field, wCoords, pcoords, vtkm::CellShapeTagLine(), worklet);
    case 3:
      return CellDerivative(field, wCoords, pcoords, vtkm::CellShapeTagTriangle(), worklet);
    case 4:
      return CellDerivative(field, wCoords, pcoords, vtkm::CellShapeTagQuad(), worklet);
  }

  vtkm::IdComponent firstPointIndex, secondPointIndex;
  PolygonComputeIndices(pcoords, numPoints, firstPointIndex, secondPointIndex);
  return PolygonDerivative(field, wCoords, numPoints, firstPointIndex, secondPointIndex);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& inputField,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagQuad,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(inputField.GetNumberOfComponents() == 4);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 4);

  using ValueType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;
  vtkm::Vec<ValueType, 4> field;
  inputField.CopyInto(field);
  vtkm::Vec<WCoordType, 4> wpoints;
  wCoords.CopyInto(wpoints);

  return detail::CellDerivativeFor2DCell(field, wpoints, pcoords, vtkm::CellShapeTagQuad());
}

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const vtkm::VecAxisAlignedPointCoordinates<2>& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagQuad,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 4);

  using T = typename FieldVecType::ComponentType;
  using VecT = vtkm::Vec<T, 2>;

  VecT pc(static_cast<typename vtkm::VecTraits<T>::ComponentType>(pcoords[0]),
          static_cast<typename vtkm::VecTraits<T>::ComponentType>(pcoords[1]));
  VecT rc = VecT(T(1)) - pc;

  VecT sum = field[0] * VecT(-rc[1], -rc[0]);
  sum = sum + field[1] * VecT(rc[1], -pc[0]);
  sum = sum + field[2] * VecT(pc[1], pc[0]);
  sum = sum + field[3] * VecT(-pc[1], rc[0]);

  return vtkm::Vec<T, 3>(static_cast<T>(sum[0] / wCoords.GetSpacing()[0]),
                         static_cast<T>(sum[1] / wCoords.GetSpacing()[1]),
                         T(0));
}

//-----------------------------------------------------------------------------
namespace detail
{

template <typename ValueType, typename LUType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TetraDerivativeFinish(
  const vtkm::Vec<ValueType, 4>& field,
  const vtkm::Matrix<LUType, 3, 3>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 3>& LUPermutation,
  vtkm::TypeTraitsScalarTag)
{
  // Finish solving linear equation. See TriangleDerivative implementation
  // for more detail.
  vtkm::Vec<LUType, 3> b(field[1] - field[0], field[2] - field[0], field[3] - field[0]);

  return vtkm::detail::MatrixLUPSolve(LUFactorization, LUPermutation, b);
}

template <typename ValueType, typename LUType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TetraDerivativeFinish(
  const vtkm::Vec<ValueType, 4>& field,
  const vtkm::Matrix<LUType, 3, 3>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 3>& LUPermutation,
  vtkm::TypeTraitsVectorTag)
{
  using FieldTraits = vtkm::VecTraits<ValueType>;
  using FieldComponentType = typename FieldTraits::ComponentType;

  vtkm::Vec<ValueType, 3> gradient(vtkm::TypeTraits<ValueType>::ZeroInitialization());

  for (vtkm::IdComponent fieldComponent = 0;
       fieldComponent < FieldTraits::GetNumberOfComponents(field[0]);
       fieldComponent++)
  {
    vtkm::Vec<FieldComponentType, 4> subField(FieldTraits::GetComponent(field[0], fieldComponent),
                                              FieldTraits::GetComponent(field[1], fieldComponent),
                                              FieldTraits::GetComponent(field[2], fieldComponent),
                                              FieldTraits::GetComponent(field[3], fieldComponent));

    vtkm::Vec<FieldComponentType, 3> subGradient =
      TetraDerivativeFinish(subField,
                            LUFactorization,
                            LUPermutation,
                            typename vtkm::TypeTraits<FieldComponentType>::DimensionalityTag());

    FieldTraits::SetComponent(gradient[0], fieldComponent, subGradient[0]);
    FieldTraits::SetComponent(gradient[1], fieldComponent, subGradient[1]);
    FieldTraits::SetComponent(gradient[2], fieldComponent, subGradient[2]);
  }

  return gradient;
}

template <typename ValueType, typename LUType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TetraDerivativeFinish(
  const vtkm::Vec<ValueType, 4>& field,
  const vtkm::Matrix<LUType, 3, 3>& LUFactorization,
  const vtkm::Vec<vtkm::IdComponent, 3>& LUPermutation,
  vtkm::TypeTraitsMatrixTag)
{
  return TetraDerivativeFinish(field, LUFactorization, LUPermutation, vtkm::TypeTraitsVectorTag());
}

template <typename ValueType, typename WorldCoordType>
VTKM_EXEC vtkm::Vec<ValueType, 3> TetraDerivative(const vtkm::Vec<ValueType, 4>& field,
                                                  const vtkm::Vec<WorldCoordType, 4>& wCoords)
{
  using BaseComponentType = typename BaseComponent<ValueType>::Type;

  // The scalar values of the four points in a tetrahedron completely specify a
  // linear field (with constant gradient). The field, defined by the 3-vector
  // gradient g and scalar value s_origin, can be found with this set of 4
  // equations and 4 unknowns.
  //
  // Dot(p0, g) + s_origin = s0
  // Dot(p1, g) + s_origin = s1
  // Dot(p2, g) + s_origin = s2
  // Dot(p3, g) + s_origin = s3
  //
  // Where the p's are point coordinates. But we don't really care about
  // s_origin. We just want to find the gradient g. With some simple
  // elimination we, we can get rid of s_origin and be left with 3 equations
  // and 3 unknowns.
  //
  // Dot(p1-p0, g) = s1 - s0
  // Dot(p2-p0, g) = s2 - s0
  // Dot(p3-p0, g) = s3 - s0
  //
  // We'll solve this by putting this in matrix form Ax = b where the rows of A
  // are the differences in points and normal, b has the scalar differences,
  // and x is really the gradient g.

  vtkm::Vec<BaseComponentType, 3> v0 = wCoords[1] - wCoords[0];
  vtkm::Vec<BaseComponentType, 3> v1 = wCoords[2] - wCoords[0];
  vtkm::Vec<BaseComponentType, 3> v2 = wCoords[3] - wCoords[0];

  vtkm::Matrix<BaseComponentType, 3, 3> A;
  vtkm::MatrixSetRow(A, 0, v0);
  vtkm::MatrixSetRow(A, 1, v1);
  vtkm::MatrixSetRow(A, 2, v2);

  // If the tetrahedron is degenerate, then valid will be false. For now we are
  // ignoring it. We could detect it if we determine we need to although I have
  // seen singular matrices missed due to floating point error.
  //
  bool valid;

  // If you look at the implementation of vtkm::SolveLinearSystem, you will see
  // that it is done in two parts. First, it does an LU factorization and then
  // uses that result to complete the solve. The factorization part talkes the
  // longest amount of time, and if we are performing the gradient on a vector
  // field, the factorization can be reused for each component of the vector
  // field. Thus, we are going to call the internals of SolveLinearSystem
  // ourselves to do the factorization and then apply it to all components.
  vtkm::Vec<vtkm::IdComponent, 3> permutation;
  BaseComponentType inversionParity; // Unused
  vtkm::detail::MatrixLUPFactor(A, permutation, inversionParity, valid);
  // MatrixLUPFactor does in place factorization. A is now the LU factorization.
  return TetraDerivativeFinish(
    field, A, permutation, typename vtkm::TypeTraits<ValueType>::DimensionalityTag());
}

} // namespace detail

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& inputField,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& vtkmNotUsed(pcoords),
  vtkm::CellShapeTagTetra,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(inputField.GetNumberOfComponents() == 4);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 4);

  using ValueType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;
  vtkm::Vec<ValueType, 4> field;
  inputField.CopyInto(field);
  vtkm::Vec<WCoordType, 4> wpoints;
  wCoords.CopyInto(wpoints);
  return detail::TetraDerivative(field, wpoints);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& inputField,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagHexahedron,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(inputField.GetNumberOfComponents() == 8);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 8);

  using ValueType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;
  vtkm::Vec<ValueType, 8> field;
  inputField.CopyInto(field);
  vtkm::Vec<WCoordType, 8> wpoints;
  wCoords.CopyInto(wpoints);

  return detail::CellDerivativeFor3DCell(field, wpoints, pcoords, vtkm::CellShapeTagHexahedron());
}

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const vtkm::VecAxisAlignedPointCoordinates<3>& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagHexahedron,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 8);

  using T = typename FieldVecType::ComponentType;
  using VecT = vtkm::Vec<T, 3>;

  VecT pc(static_cast<typename vtkm::VecTraits<T>::ComponentType>(pcoords[0]),
          static_cast<typename vtkm::VecTraits<T>::ComponentType>(pcoords[1]),
          static_cast<typename vtkm::VecTraits<T>::ComponentType>(pcoords[2]));
  VecT rc = VecT(T(1)) - pc;

  VecT sum = field[0] * VecT(-rc[1] * rc[2], -rc[0] * rc[2], -rc[0] * rc[1]);
  sum = sum + field[1] * VecT(rc[1] * rc[2], -pc[0] * rc[2], -pc[0] * rc[1]);
  sum = sum + field[2] * VecT(pc[1] * rc[2], pc[0] * rc[2], -pc[0] * pc[1]);
  sum = sum + field[3] * VecT(-pc[1] * rc[2], rc[0] * rc[2], -rc[0] * pc[1]);
  sum = sum + field[4] * VecT(-rc[1] * pc[2], -rc[0] * pc[2], rc[0] * rc[1]);
  sum = sum + field[5] * VecT(rc[1] * pc[2], -pc[0] * pc[2], pc[0] * rc[1]);
  sum = sum + field[6] * VecT(pc[1] * pc[2], pc[0] * pc[2], pc[0] * pc[1]);
  sum = sum + field[7] * VecT(-pc[1] * pc[2], rc[0] * pc[2], rc[0] * pc[1]);

  return sum / wCoords.GetSpacing();
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& inputField,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagWedge,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(inputField.GetNumberOfComponents() == 6);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 6);

  using ValueType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;
  vtkm::Vec<ValueType, 6> field;
  inputField.CopyInto(field);
  vtkm::Vec<WCoordType, 6> wpoints;
  wCoords.CopyInto(wpoints);

  return detail::CellDerivativeFor3DCell(field, wpoints, pcoords, vtkm::CellShapeTagWedge());
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& inputField,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPyramid,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(inputField.GetNumberOfComponents() == 5);
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == 5);

  using ValueType = typename FieldVecType::ComponentType;
  using WCoordType = typename WorldCoordType::ComponentType;
  vtkm::Vec<ValueType, 5> field;
  inputField.CopyInto(field);
  vtkm::Vec<WCoordType, 5> wpoints;
  wCoords.CopyInto(wpoints);

  return detail::CellDerivativeFor3DCell(field, wpoints, pcoords, vtkm::CellShapeTagPyramid());
}
}
} // namespace vtkm::exec

#endif //vtk_m_exec_Derivative_h
