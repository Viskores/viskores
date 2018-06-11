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
#ifndef vtk_m_exec_ParametricCoordinates_h
#define vtk_m_exec_ParametricCoordinates_h

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/NewtonsMethod.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/Jacobian.h>
#include <vtkm/exec/internal/FastVec.h>
#include <vtkm/internal/Assume.h>

namespace vtkm
{
namespace exec
{

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagEmpty,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 0);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagVertex,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 1);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagLine,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 2);
  pcoords[0] = 0.5;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagTriangle,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 3);
  pcoords[0] = static_cast<ParametricCoordType>(1.0 / 3.0);
  pcoords[1] = static_cast<ParametricCoordType>(1.0 / 3.0);
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagPolygon,
                                                         const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSERT(numPoints > 0);
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagVertex(), worklet);
      break;
    case 2:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagLine(), worklet);
      break;
    case 3:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagTriangle(), worklet);
      break;
    default:
      pcoords[0] = 0.5;
      pcoords[1] = 0.5;
      pcoords[2] = 0;
      break;
  }
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagQuad,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 4);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagTetra,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 4);
  pcoords[0] = 0.25;
  pcoords[1] = 0.25;
  pcoords[2] = 0.25;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagHexahedron,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 8);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0.5;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagWedge,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 6);
  pcoords[0] = static_cast<ParametricCoordType>(1.0 / 3.0);
  pcoords[1] = static_cast<ParametricCoordType>(1.0 / 3.0);
  pcoords[2] = 0.5;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagPyramid,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 5);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = static_cast<ParametricCoordType>(0.2);
}

//-----------------------------------------------------------------------------
/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagGeneric shape,
                                                         const vtkm::exec::FunctorBase& worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      ParametricCoordinatesCenter(numPoints, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesCenter.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> ParametricCoordinatesCenter(
  vtkm::IdComponent numPoints,
  CellShapeTag shape,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::Vec<vtkm::FloatDefault, 3> pcoords;
  ParametricCoordinatesCenter(numPoints, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent,
                                                        vtkm::IdComponent,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagEmpty,
                                                        const vtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Empty cell has no points.");
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagVertex,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints;  // Silence compiler warnings.
  (void)pointIndex; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 1);
  VTKM_ASSUME(pointIndex == 0);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagLine,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 2);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 2));

  pcoords[0] = static_cast<ParametricCoordType>(pointIndex);
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagTriangle,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 3);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 3));

  switch (pointIndex)
  {
    case 0:
      pcoords[0] = 0;
      pcoords[1] = 0;
      break;
    case 1:
      pcoords[0] = 1;
      pcoords[1] = 0;
      break;
    case 2:
      pcoords[0] = 0;
      pcoords[1] = 1;
      break;
  }
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagPolygon,
                                                        const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSUME((numPoints > 0));
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < numPoints));

  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, vtkm::CellShapeTagVertex(), worklet);
      return;
    case 2:
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, vtkm::CellShapeTagLine(), worklet);
      return;
    case 3:
      ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, vtkm::CellShapeTagTriangle(), worklet);
      return;
    case 4:
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, vtkm::CellShapeTagQuad(), worklet);
      return;
  }

  // If we are here, then numPoints >= 5.

  const ParametricCoordType angle =
    static_cast<ParametricCoordType>(pointIndex * 2 * vtkm::Pi() / numPoints);

  pcoords[0] = 0.5f * (vtkm::Cos(angle) + 1);
  pcoords[1] = 0.5f * (vtkm::Sin(angle) + 1);
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagQuad,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 4);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 4));

  switch (pointIndex)
  {
    case 0:
      pcoords[0] = 0;
      pcoords[1] = 0;
      break;
    case 1:
      pcoords[0] = 1;
      pcoords[1] = 0;
      break;
    case 2:
      pcoords[0] = 1;
      pcoords[1] = 1;
      break;
    case 3:
      pcoords[0] = 0;
      pcoords[1] = 1;
      break;
  }
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagTetra,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 4);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 4));

  switch (pointIndex)
  {
    case 0:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 1:
      pcoords[0] = 1;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 2:
      pcoords[0] = 0;
      pcoords[1] = 1;
      pcoords[2] = 0;
      break;
    case 3:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 1;
      break;
  }
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagHexahedron,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 8);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 8));

  switch (pointIndex)
  {
    case 0:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 1:
      pcoords[0] = 1;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 2:
      pcoords[0] = 1;
      pcoords[1] = 1;
      pcoords[2] = 0;
      break;
    case 3:
      pcoords[0] = 0;
      pcoords[1] = 1;
      pcoords[2] = 0;
      break;
    case 4:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 1;
      break;
    case 5:
      pcoords[0] = 1;
      pcoords[1] = 0;
      pcoords[2] = 1;
      break;
    case 6:
      pcoords[0] = 1;
      pcoords[1] = 1;
      pcoords[2] = 1;
      break;
    case 7:
      pcoords[0] = 0;
      pcoords[1] = 1;
      pcoords[2] = 1;
      break;
  }
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagWedge,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 6);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 6));

  switch (pointIndex)
  {
    case 0:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 1:
      pcoords[0] = 0;
      pcoords[1] = 1;
      pcoords[2] = 0;
      break;
    case 2:
      pcoords[0] = 1;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 3:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 1;
      break;
    case 4:
      pcoords[0] = 0;
      pcoords[1] = 1;
      pcoords[2] = 1;
      break;
    case 5:
      pcoords[0] = 1;
      pcoords[1] = 0;
      pcoords[2] = 1;
      break;
  }
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagPyramid,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 5);
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < 5));

  switch (pointIndex)
  {
    case 0:
      pcoords[0] = 0;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 1:
      pcoords[0] = 1;
      pcoords[1] = 0;
      pcoords[2] = 0;
      break;
    case 2:
      pcoords[0] = 1;
      pcoords[1] = 1;
      pcoords[2] = 0;
      break;
    case 3:
      pcoords[0] = 0;
      pcoords[1] = 1;
      pcoords[2] = 0;
      break;
    case 4:
      pcoords[0] = 0.5;
      pcoords[1] = 0.5;
      pcoords[2] = 1;
      break;
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagGeneric shape,
                                                        const vtkm::exec::FunctorBase& worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> ParametricCoordinatesPoint(
  vtkm::IdComponent numPoints,
  vtkm::IdComponent pointIndex,
  CellShapeTag shape,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::Vec<vtkm::FloatDefault, 3> pcoords;
  ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template <typename WorldCoordVector, typename PCoordType, typename CellShapeTag>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        CellShapeTag shape,
                                        const vtkm::exec::FunctorBase& worklet)
{
  return vtkm::exec::CellInterpolate(pointWCoords, pcoords, shape, worklet);
}

//-----------------------------------------------------------------------------
template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector&,
                                        const typename WorldCoordVector::ComponentType&,
                                        vtkm::CellShapeTagEmpty,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to find point coordinates in empty cell.");
  success = false;
  return typename WorldCoordVector::ComponentType();
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType&,
                                        vtkm::CellShapeTagVertex,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  (void)pointWCoords; // Silence compiler warnings.
  VTKM_ASSERT(pointWCoords.GetNumberOfComponents() == 1);
  success = true;
  return typename WorldCoordVector::ComponentType(0, 0, 0);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagLine,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(pointWCoords.GetNumberOfComponents() == 2);
  success = true;

  // Because this is a line, there is only one valid parametric coordinate. Let
  // vec be the vector from the first point to the second point
  // (pointWCoords[1] - pointWCoords[0]), which is the direction of the line.
  // Dot(vec,wcoords-pointWCoords[0])/mag(vec) is the orthoginal projection of
  // wcoords on the line and represents the distance between the orthoginal
  // projection and pointWCoords[0]. The parametric coordinate is the fraction
  // of this over the length of the segment, which is mag(vec). Thus, the
  // parametric coordinate is Dot(vec,wcoords-pointWCoords[0])/mag(vec)^2.

  using Vector3 = typename WorldCoordVector::ComponentType;
  using T = typename Vector3::ComponentType;

  Vector3 vec = pointWCoords[1] - pointWCoords[0];
  T numerator = vtkm::Dot(vec, wcoords - pointWCoords[0]);
  T denominator = vtkm::MagnitudeSquared(vec);

  return Vector3(numerator / denominator, 0, 0);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagTriangle,
                                        bool& success,
                                        const vtkm::exec::FunctorBase&)
{
  success = true;
  vtkm::exec::internal::FastVec<WorldCoordVector, 3> local(pointWCoords);
  return vtkm::exec::internal::ReverseInterpolateTriangle(local.Get(), wcoords);
}

//-----------------------------------------------------------------------------
namespace detail
{

template <typename WorldCoordVector, typename CellShapeTag>
class JacobianFunctorQuad
{
  using T = typename WorldCoordVector::ComponentType::ComponentType;
  using Vector2 = vtkm::Vec<T, 2>;
  using Matrix2x2 = vtkm::Matrix<T, 2, 2>;
  using SpaceType = vtkm::exec::internal::Space2D<T>;

  const WorldCoordVector* PointWCoords;
  const SpaceType* Space;

public:
  VTKM_EXEC
  JacobianFunctorQuad(const WorldCoordVector* pointWCoords, const SpaceType* space)
    : PointWCoords(pointWCoords)
    , Space(space)
  {
  }

  VTKM_EXEC
  Matrix2x2 operator()(const Vector2& pcoords) const
  {
    Matrix2x2 jacobian;
    vtkm::exec::JacobianFor2DCell(*this->PointWCoords,
                                  vtkm::Vec<T, 3>(pcoords[0], pcoords[1], 0),
                                  *this->Space,
                                  jacobian,
                                  CellShapeTag());
    return jacobian;
  }
};

template <typename WorldCoordVector, typename CellShapeTag>
class CoordinatesFunctorQuad
{
  using T = typename WorldCoordVector::ComponentType::ComponentType;
  using Vector2 = vtkm::Vec<T, 2>;
  using Vector3 = vtkm::Vec<T, 3>;
  using SpaceType = vtkm::exec::internal::Space2D<T>;

  const WorldCoordVector* PointWCoords;
  const SpaceType* Space;
  const vtkm::exec::FunctorBase* Worklet;

public:
  VTKM_EXEC
  CoordinatesFunctorQuad(const WorldCoordVector* pointWCoords,
                         const SpaceType* space,
                         const vtkm::exec::FunctorBase* worklet)
    : PointWCoords(pointWCoords)
    , Space(space)
    , Worklet(worklet)
  {
  }

  VTKM_EXEC
  Vector2 operator()(Vector2 pcoords) const
  {
    Vector3 pcoords3D(pcoords[0], pcoords[1], 0);
    Vector3 wcoords = vtkm::exec::ParametricCoordinatesToWorldCoordinates(
      *this->PointWCoords, pcoords3D, CellShapeTag(), *this->Worklet);
    return this->Space->ConvertCoordToSpace(wcoords);
  }
};

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinatesQuad(const WorldCoordVector& pointWCoords,
                                            const typename WorldCoordVector::ComponentType& wcoords,
                                            bool& success,
                                            const vtkm::exec::FunctorBase& worklet)
{
  using T = typename WorldCoordVector::ComponentType::ComponentType;
  using Vector2 = vtkm::Vec<T, 2>;
  using Vector3 = vtkm::Vec<T, 3>;

  // We have an underdetermined system in 3D, so create a 2D space in the
  // plane that the polygon sits.
  vtkm::exec::internal::Space2D<T> space(pointWCoords[0], pointWCoords[1], pointWCoords[3]);

  auto result = vtkm::NewtonsMethod(
    JacobianFunctorQuad<WorldCoordVector, vtkm::CellShapeTagQuad>(&pointWCoords, &space),
    CoordinatesFunctorQuad<WorldCoordVector, vtkm::CellShapeTagQuad>(
      &pointWCoords, &space, &worklet),
    space.ConvertCoordToSpace(wcoords),
    Vector2(0.5f, 0.5f));

  success = result.Valid;
  return Vector3(result.Solution[0], result.Solution[1], 0);
}
} // namespace detail

static inline VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> WorldCoordinatesToParametricCoordinates(
  const vtkm::VecAxisAlignedPointCoordinates<2>& pointWCoords,
  const vtkm::Vec<vtkm::FloatDefault, 3>& wcoords,
  vtkm::CellShapeTagQuad,
  bool& success,
  const FunctorBase&)
{
  success = true;
  return (wcoords - pointWCoords.GetOrigin()) / pointWCoords.GetSpacing();
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagQuad,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  vtkm::exec::internal::FastVec<WorldCoordVector, 4> local(pointWCoords);
  return detail::WorldCoordinatesToParametricCoordinatesQuad(
    local.Get(), wcoords, success, worklet);
}

//-----------------------------------------------------------------------------
template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagPolygon,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  const vtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();
  VTKM_ASSERT(numPoints > 0);
  switch (numPoints)
  {
    case 1:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, vtkm::CellShapeTagVertex(), success, worklet);
    case 2:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, vtkm::CellShapeTagLine(), success, worklet);
    case 3:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, vtkm::CellShapeTagTriangle(), success, worklet);
    case 4:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, vtkm::CellShapeTagQuad(), success, worklet);
  }

  // If we are here, then there are 5 or more points on this polygon.

  // Arrange the points such that they are on the circle circumscribed in the
  // unit square from 0 to 1. That is, the point are on the circle centered at
  // coordinate 0.5,0.5 with radius 0.5. The polygon is divided into regions
  // defined by they triangle fan formed by the points around the center. This
  // is C0 continuous but not necessarily C1 continuous. It is also possible to
  // have a non 1 to 1 mapping between parametric coordinates world coordinates
  // if the polygon is not planar or convex.

  using WCoordType = typename WorldCoordVector::ComponentType;

  // Find the position of the center point.
  WCoordType wcoordCenter = pointWCoords[0];
  for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
  {
    wcoordCenter = wcoordCenter + pointWCoords[pointIndex];
  }
  wcoordCenter = wcoordCenter * WCoordType(1.0f / static_cast<float>(numPoints));

  // Find the normal vector to the polygon. If the polygon is planar, convex,
  // and in general position, any three points will give a normal in the same
  // direction. Although not perfectly robust, we can reduce the effect of
  // non-planar, non-convex, or degenerate polygons by picking three points
  // topologically far from each other. Note that we do not care about the
  // length of the normal in this case.
  WCoordType polygonNormal;
  {
    WCoordType vec1 = pointWCoords[numPoints / 3] - pointWCoords[0];
    WCoordType vec2 = pointWCoords[2 * numPoints / 3] - pointWCoords[1];
    polygonNormal = vtkm::Cross(vec1, vec2);
  }

  // Find which triangle wcoords is located in. We do this by defining the
  // equations for the planes through the radial edges and perpendicular to the
  // polygon. The point is in the triangle if it is on the correct side of both
  // planes.
  vtkm::IdComponent firstPointIndex;
  vtkm::IdComponent secondPointIndex = 0;
  bool foundTriangle = false;
  for (firstPointIndex = 0; firstPointIndex < numPoints - 1; firstPointIndex++)
  {
    WCoordType vecInPlane = pointWCoords[firstPointIndex] - wcoordCenter;
    WCoordType planeNormal = vtkm::Cross(polygonNormal, vecInPlane);
    typename WCoordType::ComponentType planeOffset = vtkm::Dot(planeNormal, wcoordCenter);
    if (vtkm::Dot(planeNormal, wcoords) < planeOffset)
    {
      // wcoords on wrong side of plane, thus outside of triangle
      continue;
    }

    secondPointIndex = firstPointIndex + 1;
    vecInPlane = pointWCoords[secondPointIndex] - wcoordCenter;
    planeNormal = vtkm::Cross(polygonNormal, vecInPlane);
    planeOffset = vtkm::Dot(planeNormal, wcoordCenter);
    if (vtkm::Dot(planeNormal, wcoords) > planeOffset)
    {
      // wcoords on wrong side of plane, thus outside of triangle
      continue;
    }

    foundTriangle = true;
    break;
  }
  if (!foundTriangle)
  {
    // wcoord was outside of all triangles we checked. It must be inside the
    // one triangle we did not check (the one between the first and last
    // polygon points).
    firstPointIndex = numPoints - 1;
    secondPointIndex = 0;
  }

  // Build a structure containing the points of the triangle wcoords is in and
  // use the triangle version of this function to find the parametric
  // coordinates.
  vtkm::Vec<WCoordType, 3> triangleWCoords;
  triangleWCoords[0] = wcoordCenter;
  triangleWCoords[1] = pointWCoords[firstPointIndex];
  triangleWCoords[2] = pointWCoords[secondPointIndex];

  WCoordType trianglePCoords = WorldCoordinatesToParametricCoordinates(
    triangleWCoords, wcoords, vtkm::CellShapeTagTriangle(), success, worklet);

  // trianglePCoords is in the triangle's parameter space rather than the
  // polygon's parameter space. We can find the polygon's parameter space by
  // repurposing the ParametricCoordinatesToWorldCoordinates by using the
  // polygon parametric coordinates as a proxy for world coordinates.
  triangleWCoords[0] = WCoordType(0.5f, 0.5f, 0);
  ParametricCoordinatesPoint(
    numPoints, firstPointIndex, triangleWCoords[1], vtkm::CellShapeTagPolygon(), worklet);
  ParametricCoordinatesPoint(
    numPoints, secondPointIndex, triangleWCoords[2], vtkm::CellShapeTagPolygon(), worklet);
  return ParametricCoordinatesToWorldCoordinates(
    triangleWCoords, trianglePCoords, vtkm::CellShapeTagTriangle(), worklet);
}

//-----------------------------------------------------------------------------
namespace detail
{

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinatesTetra(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords)
{
  // We solve the world to parametric coordinates problem for tetrahedra
  // similarly to that for triangles. Before understanding this code, you
  // should understand the triangle code (in ReverseInterpolateTriangle in
  // CellInterpolate.h). Go ahead. Read it now.
  //
  // The tetrahedron code is an obvious extension of the triangle code by
  // considering the parallelpiped formed by wcoords and p0 of the triangle
  // and the three adjacent faces.  This parallelpiped is equivalent to the
  // axis-aligned cuboid anchored at the origin of parametric space.
  //
  // Just like the triangle, we compute the parametric coordinate for each axis
  // by intersecting a plane with each edge emanating from p0. The plane is
  // defined by the one that goes through wcoords (duh) and is parallel to the
  // plane formed by the other two edges emanating from p0 (as dictated by the
  // aforementioned parallelpiped).
  //
  // In review, by parameterizing the line by fraction of distance the distance
  // from p0 to the adjacent point (which is itself the parametric coordinate
  // we are after), we get the following definition for the intersection.
  //
  // d = Dot((wcoords - p0), planeNormal)/Dot((p1-p0), planeNormal)
  //

  const auto vec0 = pointWCoords[1] - pointWCoords[0];
  const auto vec1 = pointWCoords[2] - pointWCoords[0];
  const auto vec2 = pointWCoords[3] - pointWCoords[0];
  const auto coordVec = wcoords - pointWCoords[0];

  typename WorldCoordVector::ComponentType pcoords;
  auto planeNormal = vtkm::Cross(vec1, vec2);
  pcoords[0] = vtkm::Dot(coordVec, planeNormal) / vtkm::Dot(vec0, planeNormal);

  planeNormal = vtkm::Cross(vec0, vec2);
  pcoords[1] = vtkm::Dot(coordVec, planeNormal) / vtkm::Dot(vec1, planeNormal);

  planeNormal = vtkm::Cross(vec0, vec1);
  pcoords[2] = vtkm::Dot(coordVec, planeNormal) / vtkm::Dot(vec2, planeNormal);

  return pcoords;
}
} // detail

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagTetra,
                                        bool& success,
                                        const vtkm::exec::FunctorBase&)
{
  success = true;
  vtkm::exec::internal::FastVec<WorldCoordVector, 4> local(pointWCoords);
  return detail::WorldCoordinatesToParametricCoordinatesTetra(local.Get(), wcoords);
}

//-----------------------------------------------------------------------------
namespace detail
{

template <typename WorldCoordVector, typename CellShapeTag>
class JacobianFunctor3DCell
{
  using T = typename WorldCoordVector::ComponentType::ComponentType;
  using Vector3 = vtkm::Vec<T, 3>;
  using Matrix3x3 = vtkm::Matrix<T, 3, 3>;

  const WorldCoordVector* PointWCoords;

public:
  VTKM_EXEC
  JacobianFunctor3DCell(const WorldCoordVector* pointWCoords)
    : PointWCoords(pointWCoords)
  {
  }

  VTKM_EXEC
  Matrix3x3 operator()(const Vector3& pcoords) const
  {
    Matrix3x3 jacobian;
    vtkm::exec::JacobianFor3DCell(*this->PointWCoords, pcoords, jacobian, CellShapeTag());
    return jacobian;
  }
};

template <typename WorldCoordVector, typename CellShapeTag>
class CoordinatesFunctor3DCell
{
  using T = typename WorldCoordVector::ComponentType::ComponentType;
  using Vector3 = vtkm::Vec<T, 3>;

  const WorldCoordVector* PointWCoords;
  const vtkm::exec::FunctorBase* Worklet;

public:
  VTKM_EXEC
  CoordinatesFunctor3DCell(const WorldCoordVector* pointWCoords,
                           const vtkm::exec::FunctorBase* worklet)
    : PointWCoords(pointWCoords)
    , Worklet(worklet)
  {
  }

  VTKM_EXEC
  Vector3 operator()(Vector3 pcoords) const
  {
    return vtkm::exec::ParametricCoordinatesToWorldCoordinates(
      *this->PointWCoords, pcoords, CellShapeTag(), *this->Worklet);
  }
};

template <typename WorldCoordVector, typename CellShapeTag>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates3D(const WorldCoordVector& pointWCoords,
                                          const typename WorldCoordVector::ComponentType& wcoords,
                                          CellShapeTag,
                                          bool& success,
                                          const vtkm::exec::FunctorBase& worklet)
{
  auto result = vtkm::NewtonsMethod(
    JacobianFunctor3DCell<WorldCoordVector, CellShapeTag>(&pointWCoords),
    CoordinatesFunctor3DCell<WorldCoordVector, CellShapeTag>(&pointWCoords, &worklet),
    wcoords,
    typename WorldCoordVector::ComponentType(0.5f, 0.5f, 0.5f));
  success = result.Valid;
  return result.Solution;
}
} // detail

static inline VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> WorldCoordinatesToParametricCoordinates(
  const vtkm::VecAxisAlignedPointCoordinates<3>& pointWCoords,
  const vtkm::Vec<vtkm::FloatDefault, 3>& wcoords,
  vtkm::CellShapeTagHexahedron,
  bool& success,
  const FunctorBase&)
{
  success = true;
  return (wcoords - pointWCoords.GetOrigin()) / pointWCoords.GetSpacing();
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagHexahedron,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  vtkm::exec::internal::FastVec<WorldCoordVector, 8> local(pointWCoords);
  return detail::WorldCoordinatesToParametricCoordinates3D(
    local.Get(), wcoords, vtkm::CellShapeTagHexahedron(), success, worklet);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagWedge,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  vtkm::exec::internal::FastVec<WorldCoordVector, 6> local(pointWCoords);
  return detail::WorldCoordinatesToParametricCoordinates3D(
    local.Get(), wcoords, vtkm::CellShapeTagWedge(), success, worklet);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagPyramid,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  vtkm::exec::internal::FastVec<WorldCoordVector, 5> local(pointWCoords);
  return detail::WorldCoordinatesToParametricCoordinates3D(
    local.Get(), wcoords, vtkm::CellShapeTagPyramid(), success, worklet);
}

//-----------------------------------------------------------------------------
template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagGeneric shape,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType result;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(result = WorldCoordinatesToParametricCoordinates(
                                pointWCoords, wcoords, CellShapeTag(), success, worklet));
    default:
      success = false;
      worklet.RaiseError("Unknown cell shape sent to world 2 parametric.");
      return typename WorldCoordVector::ComponentType();
  }

  return result;
}
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ParametricCoordinates_h
