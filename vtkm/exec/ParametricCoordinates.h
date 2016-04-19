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
#ifndef vtk_m_exec_ParametricCoordinates_h
#define vtk_m_exec_ParametricCoordinates_h

#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/NewtonsMethod.h>
#include <vtkm/VecRectilinearPointCoordinates.h>
#include <vtkm/exec/Assert.h>
#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>

namespace vtkm {
namespace exec {

//-----------------------------------------------------------------------------
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagEmpty,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 0, worklet);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagVertex,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 1, worklet);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagLine,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 2, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagTriangle,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 3, worklet);
  pcoords[0] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[1] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagPolygon,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints > 0, worklet);
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(
            numPoints, pcoords, vtkm::CellShapeTagVertex(), worklet);
      break;
    case 2:
      ParametricCoordinatesCenter(
            numPoints, pcoords, vtkm::CellShapeTagLine(), worklet);
      break;
    case 3:
      ParametricCoordinatesCenter(
            numPoints, pcoords, vtkm::CellShapeTagTriangle(), worklet);
      break;
    default:
      pcoords[0] = 0.5;
      pcoords[1] = 0.5;
      pcoords[2] = 0;
      break;
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagQuad,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagTetra,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);
  pcoords[0] = 0.25;
  pcoords[1] = 0.25;
  pcoords[2] = 0.25;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagHexahedron,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 8, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0.5;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagWedge,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 6, worklet);
  pcoords[0] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[1] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[2] = 0.5;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagPyramid,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 5, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = static_cast<ParametricCoordType>(0.2);
}

//-----------------------------------------------------------------------------
/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 vtkm::CellShapeTagGeneric shape,
                                 const vtkm::exec::FunctorBase &worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(ParametricCoordinatesCenter(numPoints,
                                                          pcoords,
                                                          CellShapeTag(),
                                                          worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesCenter.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template<typename CellShapeTag>
VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                            CellShapeTag shape,
                            const vtkm::exec::FunctorBase &worklet)
{
  vtkm::Vec<vtkm::FloatDefault,3> pcoords;
  ParametricCoordinatesCenter(numPoints, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent,
                                vtkm::IdComponent,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagEmpty,
                                const vtkm::exec::FunctorBase &worklet)
{
  worklet.RaiseError("Empty cell has no points.");
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagVertex,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 1, worklet);
  VTKM_ASSERT_EXEC(pointIndex == 0, worklet);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagLine,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 2, worklet);
  VTKM_ASSERT_EXEC((pointIndex >= 0) && (pointIndex < 2), worklet);

  pcoords[0] = static_cast<ParametricCoordType>(pointIndex);
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagTriangle,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 3, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; break;
    case 2: pcoords[0] = 0; pcoords[1] = 1; break;
    default: worklet.RaiseError("Bad point index.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagPolygon,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints > 0, worklet);

  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(numPoints,
                                 pointIndex,
                                 pcoords,
                                 vtkm::CellShapeTagVertex(),
                                 worklet);
      return;
    case 2:
      ParametricCoordinatesPoint(numPoints,
                                 pointIndex,
                                 pcoords,
                                 vtkm::CellShapeTagLine(),
                                 worklet);
      return;
    case 3:
      ParametricCoordinatesPoint(numPoints,
                                 pointIndex,
                                 pcoords,
                                 vtkm::CellShapeTagTriangle(),
                                 worklet);
      return;
    case 4:
      ParametricCoordinatesPoint(numPoints,
                                 pointIndex,
                                 pcoords,
                                 vtkm::CellShapeTagQuad(),
                                 worklet);
      return;
  }

  // If we are here, then numPoints >= 5.

  const ParametricCoordType angle =
      static_cast<ParametricCoordType>(pointIndex*2*vtkm::Pi()/numPoints);

  pcoords[0] = 0.5f*(vtkm::Cos(angle)+1);
  pcoords[1] = 0.5f*(vtkm::Sin(angle)+1);
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagQuad,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; break;
    case 2: pcoords[0] = 1; pcoords[1] = 1; break;
    case 3: pcoords[0] = 0; pcoords[1] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagTetra,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 0; break;
    case 2: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 0; break;
    case 3: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagHexahedron,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 8, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 0; break;
    case 2: pcoords[0] = 1; pcoords[1] = 1; pcoords[2] = 0; break;
    case 3: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 0; break;
    case 4: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 1; break;
    case 5: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 1; break;
    case 6: pcoords[0] = 1; pcoords[1] = 1; pcoords[2] = 1; break;
    case 7: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagWedge,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 6, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 0; break;
    case 1: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 0; break;
    case 2: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 0; break;
    case 3: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 1; break;
    case 4: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 1; break;
    case 5: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagPyramid,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 5, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 0; break;
    case 2: pcoords[0] = 1; pcoords[1] = 1; pcoords[2] = 0; break;
    case 3: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 0; break;
    case 4: pcoords[0] = 0.5; pcoords[1] = 0.5; pcoords[2] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                vtkm::CellShapeTagGeneric shape,
                                const vtkm::exec::FunctorBase &worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(ParametricCoordinatesPoint(numPoints,
                                                         pointIndex,
                                                         pcoords,
                                                         CellShapeTag(),
                                                         worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template<typename CellShapeTag>
VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                           vtkm::IdComponent pointIndex,
                           CellShapeTag shape,
                           const vtkm::exec::FunctorBase &worklet)
{
  vtkm::Vec<vtkm::FloatDefault,3> pcoords;
  ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template<typename WorldCoordVector, typename PCoordType, typename CellShapeTag>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(
    const WorldCoordVector &pointWCoords,
    const vtkm::Vec<PCoordType,3> &pcoords,
    CellShapeTag shape,
    const vtkm::exec::FunctorBase &worklet)
{
  return vtkm::exec::CellInterpolate(pointWCoords, pcoords, shape, worklet);
}

//-----------------------------------------------------------------------------

namespace detail {

template<typename WorldCoordVector, typename CellShapeTag>
class JacobianFunctorQuad
{
  typedef typename WorldCoordVector::ComponentType::ComponentType T;
  typedef vtkm::Vec<T,2> Vector2;
  typedef vtkm::Matrix<T,2,2> Matrix2x2;
  typedef vtkm::exec::internal::Space2D<T> SpaceType;

  const WorldCoordVector *PointWCoords;
  const SpaceType *Space;

public:
  VTKM_EXEC_EXPORT
  JacobianFunctorQuad(
      const WorldCoordVector *pointWCoords,
      const SpaceType *space)
    : PointWCoords(pointWCoords), Space(space)
  {  }

  VTKM_EXEC_EXPORT
  Matrix2x2 operator()(const Vector2 &pcoords) const
  {
    Matrix2x2 jacobian;
    vtkm::exec::internal::JacobianFor2DCell(
          *this->PointWCoords,
          vtkm::Vec<T,3>(pcoords[0],pcoords[1],0),
          *this->Space,
          jacobian,
          CellShapeTag());
    return jacobian;
  }
};

template<typename WorldCoordVector, typename CellShapeTag>
class CoordinatesFunctorQuad
{
  typedef typename WorldCoordVector::ComponentType::ComponentType T;
  typedef vtkm::Vec<T,2> Vector2;
  typedef vtkm::Vec<T,3> Vector3;
  typedef vtkm::exec::internal::Space2D<T> SpaceType;

  const WorldCoordVector *PointWCoords;
  const SpaceType *Space;
  const vtkm::exec::FunctorBase *Worklet;

public:
  VTKM_EXEC_EXPORT
  CoordinatesFunctorQuad(
      const WorldCoordVector *pointWCoords,
      const SpaceType *space,
      const vtkm::exec::FunctorBase *worklet)
    : PointWCoords(pointWCoords), Space(space), Worklet(worklet)
  {  }

  VTKM_EXEC_EXPORT
  Vector2 operator()(Vector2 pcoords) const {
    Vector3 pcoords3D(pcoords[0], pcoords[1], 0);
    Vector3 wcoords =
        vtkm::exec::ParametricCoordinatesToWorldCoordinates(
          *this->PointWCoords, pcoords3D, CellShapeTag(), *this->Worklet);
    return this->Space->ConvertCoordToSpace(wcoords);
  }
};

template<typename WorldCoordVector, typename CellShapeTag>
class JacobianFunctor3DCell
{
  typedef typename WorldCoordVector::ComponentType::ComponentType T;
  typedef vtkm::Vec<T,3> Vector3;
  typedef vtkm::Matrix<T,3,3> Matrix3x3;

  const WorldCoordVector *PointWCoords;

public:
  VTKM_EXEC_EXPORT
  JacobianFunctor3DCell(
      const WorldCoordVector *pointWCoords)
    : PointWCoords(pointWCoords)
  {  }

  VTKM_EXEC_EXPORT
  Matrix3x3 operator()(const Vector3 &pcoords) const
  {
    Matrix3x3 jacobian;
    vtkm::exec::internal::JacobianFor3DCell(*this->PointWCoords,
                                            pcoords,
                                            jacobian,
                                            CellShapeTag());
    return jacobian;
  }
};

template<typename WorldCoordVector, typename CellShapeTag>
class CoordinatesFunctor3DCell
{
  typedef typename WorldCoordVector::ComponentType::ComponentType T;
  typedef vtkm::Vec<T,3> Vector3;

  const WorldCoordVector *PointWCoords;
  const vtkm::exec::FunctorBase *Worklet;

public:
  VTKM_EXEC_EXPORT
  CoordinatesFunctor3DCell(const WorldCoordVector *pointWCoords,
                           const vtkm::exec::FunctorBase *worklet)
    : PointWCoords(pointWCoords), Worklet(worklet)
  {  }

  VTKM_EXEC_EXPORT
  Vector3 operator()(Vector3 pcoords) const {
    return
        vtkm::exec::ParametricCoordinatesToWorldCoordinates(
          *this->PointWCoords, pcoords, CellShapeTag(), *this->Worklet);
  }
};

template<typename WorldCoordVector, typename CellShapeTag>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates3D(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    CellShapeTag,
    const vtkm::exec::FunctorBase &worklet)
{
  return vtkm::NewtonsMethod(
        JacobianFunctor3DCell<WorldCoordVector,CellShapeTag>(&pointWCoords),
        CoordinatesFunctor3DCell<WorldCoordVector,CellShapeTag>(&pointWCoords, &worklet),
        wcoords,
        typename WorldCoordVector::ComponentType(0.5f,0.5f,0.5f));
}


} // namespace detail

//-----------------------------------------------------------------------------
template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagGeneric shape,
    const vtkm::exec::FunctorBase &worklet)
{
  typename WorldCoordVector::ComponentType result;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
          result = WorldCoordinatesToParametricCoordinates(pointWCoords,
                                                           wcoords,
                                                           CellShapeTag(),
                                                           worklet));
    default:
      worklet.RaiseError("Unknown cell shape sent to world 2 parametric.");
      return typename WorldCoordVector::ComponentType();
  }

  return result;
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &,
    const typename WorldCoordVector::ComponentType &,
    vtkm::CellShapeTagEmpty,
    const vtkm::exec::FunctorBase &worklet)
{
  worklet.RaiseError("Attempted to find point coordinates in empty cell.");
  return typename WorldCoordVector::ComponentType();
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &,
    vtkm::CellShapeTagVertex,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 1, worklet);
  return typename WorldCoordVector::ComponentType(0, 0, 0);
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagLine,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 2, worklet);

  // Because this is a line, there is only one vaild parametric coordinate. Let
  // vec be the vector from the first point to the second point
  // (pointWCoords[1] - pointWCoords[0]), which is the direction of the line.
  // dot(vec,wcoords-pointWCoords[0])/mag(vec) is the orthoginal projection of
  // wcoords on the line and represents the distance between the orthoginal
  // projection and pointWCoords[0]. The parametric coordinate is the fraction
  // of this over the length of the segment, which is mag(vec). Thus, the
  // parametric coordinate is dot(vec,wcoords-pointWCoords[0])/mag(vec)^2.

  typedef typename WorldCoordVector::ComponentType Vector3;
  typedef typename Vector3::ComponentType T;

  Vector3 vec = pointWCoords[1] - pointWCoords[0];
  T numerator = vtkm::dot(vec, wcoords - pointWCoords[0]);
  T denominator = vtkm::MagnitudeSquared(vec);

  return Vector3(numerator/denominator, 0, 0);
}

VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
WorldCoordinatesToParametricCoordinates(
    const vtkm::VecRectilinearPointCoordinates<1> &pointWCoords,
    const vtkm::Vec<vtkm::FloatDefault,3> &wcoords,
    vtkm::CellShapeTagLine,
    const FunctorBase &)
{
  return (wcoords - pointWCoords.GetOrigin())/pointWCoords.GetSpacing();
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagTriangle,
    const vtkm::exec::FunctorBase &worklet)
{
  return vtkm::exec::internal::ReverseInterpolateTriangle(
        pointWCoords, wcoords, worklet);
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagPolygon,
    const vtkm::exec::FunctorBase &worklet)
{
  const vtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();
  VTKM_ASSERT_EXEC(numPoints > 0, worklet);
  switch (numPoints)
  {
    case 1:
      return WorldCoordinatesToParametricCoordinates(
            pointWCoords, wcoords, vtkm::CellShapeTagVertex(), worklet);
    case 2:
      return WorldCoordinatesToParametricCoordinates(
            pointWCoords, wcoords, vtkm::CellShapeTagLine(), worklet);
    case 3:
      return WorldCoordinatesToParametricCoordinates(
            pointWCoords, wcoords, vtkm::CellShapeTagTriangle(), worklet);
    case 4:
      return WorldCoordinatesToParametricCoordinates(
            pointWCoords, wcoords, vtkm::CellShapeTagQuad(), worklet);
  }

  // If we are here, then there are 5 or more points on this polygon.

  // Arrange the points such that they are on the circle circumscribed in the
  // unit square from 0 to 1. That is, the point are on the circle centered at
  // coordinate 0.5,0.5 with radius 0.5. The polygon is divided into regions
  // defined by they triangle fan formed by the points around the center. This
  // is C0 continuous but not necessarily C1 continuous. It is also possible to
  // have a non 1 to 1 mapping between parametric coordinates world coordinates
  // if the polygon is not planar or convex.

  typedef typename WorldCoordVector::ComponentType WCoordType;

  // Find the position of the center point.
  WCoordType wcoordCenter = pointWCoords[0];
  for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
  {
    wcoordCenter = wcoordCenter + pointWCoords[pointIndex];
  }
  wcoordCenter = wcoordCenter*WCoordType(1.0f/static_cast<float>(numPoints));

  // Find the normal vector to the polygon. If the polygon is planar, convex,
  // and in general position, any three points will give a normal in the same
  // direction. Although not perfectly robust, we can reduce the effect of
  // non-planar, non-convex, or degenerate polygons by picking three points
  // topologically far from each other. Note that we do not care about the
  // length of the normal in this case.
  WCoordType polygonNormal;
  {
    WCoordType vec1 = pointWCoords[numPoints/3] - pointWCoords[0];
    WCoordType vec2 = pointWCoords[2*numPoints/3] - pointWCoords[1];
    polygonNormal = vtkm::Cross(vec1, vec2);
  }

  // Find which triangle wcoords is located in. We do this by defining the
  // equations for the planes through the radial edges and perpendicular to the
  // polygon. The point is in the triangle if it is on the correct side of both
  // planes.
  vtkm::IdComponent firstPointIndex;
  vtkm::IdComponent secondPointIndex;
  bool foundTriangle = false;
  for (firstPointIndex = 0; firstPointIndex < numPoints-1; firstPointIndex++)
  {
    WCoordType vecInPlane = pointWCoords[firstPointIndex] - wcoordCenter;
    WCoordType planeNormal = vtkm::Cross(polygonNormal, vecInPlane);
    typename WCoordType::ComponentType planeOffset =
        vtkm::dot(planeNormal,wcoordCenter);
    if (vtkm::dot(planeNormal,wcoords) < planeOffset)
    {
      // wcoords on wrong side of plane, thus outside of triangle
      continue;
    }

    secondPointIndex = firstPointIndex+1;
    vecInPlane = pointWCoords[secondPointIndex] - wcoordCenter;
    planeNormal = vtkm::Cross(polygonNormal, vecInPlane);
    planeOffset = vtkm::dot(planeNormal,wcoordCenter);
    if (vtkm::dot(planeNormal,wcoords) > planeOffset)
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
    firstPointIndex = numPoints-1;
    secondPointIndex = 0;
  }

  // Build a structure containing the points of the triangle wcoords is in and
  // use the triangle version of this function to find the parametric
  // coordinates.
  vtkm::Vec<WCoordType,3> triangleWCoords;
  triangleWCoords[0] = wcoordCenter;
  triangleWCoords[1] = pointWCoords[firstPointIndex];
  triangleWCoords[2] = pointWCoords[secondPointIndex];

  WCoordType trianglePCoords =
      WorldCoordinatesToParametricCoordinates(triangleWCoords,
                                              wcoords,
                                              vtkm::CellShapeTagTriangle(),
                                              worklet);

  // trianglePCoords is in the triangle's parameter space rather than the
  // polygon's parameter space. We can find the polygon's parameter space by
  // repurposing the ParametricCoordinatesToWorldCoordinates by using the
  // polygon parametric coordinates as a proxy for world coordinates.
  triangleWCoords[0] = WCoordType(0.5f, 0.5f, 0);
  ParametricCoordinatesPoint(numPoints,
                             firstPointIndex,
                             triangleWCoords[1],
                             vtkm::CellShapeTagPolygon(),
                             worklet);
  ParametricCoordinatesPoint(numPoints,
                             secondPointIndex,
                             triangleWCoords[2],
                             vtkm::CellShapeTagPolygon(),
                             worklet);
  return ParametricCoordinatesToWorldCoordinates(triangleWCoords,
                                                 trianglePCoords,
                                                 vtkm::CellShapeTagTriangle(),
                                                 worklet);
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagQuad,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 4, worklet);

  typedef typename WorldCoordVector::ComponentType::ComponentType T;
  typedef vtkm::Vec<T,2> Vector2;
  typedef vtkm::Vec<T,3> Vector3;

  // We have an underdetermined system in 3D, so create a 2D space in the
  // plane that the polygon sits.
  vtkm::exec::internal::Space2D<T> space(
        pointWCoords[0], pointWCoords[1], pointWCoords[3]);

  Vector2 pcoords =
      vtkm::NewtonsMethod(
        detail::JacobianFunctorQuad<WorldCoordVector,vtkm::CellShapeTagQuad>(&pointWCoords, &space),
        detail::CoordinatesFunctorQuad<WorldCoordVector,vtkm::CellShapeTagQuad>(&pointWCoords, &space, &worklet),
        space.ConvertCoordToSpace(wcoords),
        Vector2(0.5f, 0.5f));

  return Vector3(pcoords[0], pcoords[1], 0);
}

VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
WorldCoordinatesToParametricCoordinates(
    const vtkm::VecRectilinearPointCoordinates<2> &pointWCoords,
    const vtkm::Vec<vtkm::FloatDefault,3> &wcoords,
    vtkm::CellShapeTagQuad,
    const FunctorBase &)
{
  return (wcoords - pointWCoords.GetOrigin())/pointWCoords.GetSpacing();
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagTetra,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 4, worklet);

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
  // d = dot((wcoords - p0), planeNormal)/dot((p1-p0), planeNormal)
  //

  typedef typename WorldCoordVector::ComponentType Vector3;

  Vector3 pcoords;

  const Vector3 vec0 = pointWCoords[1] - pointWCoords[0];
  const Vector3 vec1 = pointWCoords[2] - pointWCoords[0];
  const Vector3 vec2 = pointWCoords[3] - pointWCoords[0];
  const Vector3 coordVec = wcoords - pointWCoords[0];

  Vector3 planeNormal = vtkm::Cross(vec1, vec2);
  pcoords[0] = vtkm::dot(coordVec, planeNormal)/vtkm::dot(vec0, planeNormal);

  planeNormal = vtkm::Cross(vec0, vec2);
  pcoords[1] = vtkm::dot(coordVec, planeNormal)/vtkm::dot(vec1, planeNormal);

  planeNormal = vtkm::Cross(vec0, vec1);
  pcoords[2] = vtkm::dot(coordVec, planeNormal)/vtkm::dot(vec2, planeNormal);

  return pcoords;
}


template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagHexahedron,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 8, worklet);

  return detail::WorldCoordinatesToParametricCoordinates3D(
        pointWCoords, wcoords, vtkm::CellShapeTagHexahedron(), worklet);
}

VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
WorldCoordinatesToParametricCoordinates(
    const vtkm::VecRectilinearPointCoordinates<3> &pointWCoords,
    const vtkm::Vec<vtkm::FloatDefault,3> &wcoords,
    vtkm::CellShapeTagHexahedron,
    const FunctorBase &)
{
  return (wcoords - pointWCoords.GetOrigin())/pointWCoords.GetSpacing();
}

template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagWedge,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 6, worklet);

  return detail::WorldCoordinatesToParametricCoordinates3D(
        pointWCoords, wcoords, vtkm::CellShapeTagWedge(), worklet);
}


template<typename WorldCoordVector>
VTKM_EXEC_EXPORT
typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(
    const WorldCoordVector &pointWCoords,
    const typename WorldCoordVector::ComponentType &wcoords,
    vtkm::CellShapeTagPyramid,
    const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(pointWCoords.GetNumberOfComponents() == 5, worklet);

  return detail::WorldCoordinatesToParametricCoordinates3D(
        pointWCoords, wcoords, vtkm::CellShapeTagPyramid(), worklet);
}


}
} // namespace vtkm::exec

#endif //vtk_m_exec_ParametricCoordinates_h
