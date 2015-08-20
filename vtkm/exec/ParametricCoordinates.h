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
#include <vtkm/exec/Assert.h>
#include <vtkm/exec/FunctorBase.h>

namespace vtkm {
namespace exec {

//-----------------------------------------------------------------------------
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagEmpty,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 0, worklet);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagVertex,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 1, worklet);
  pcoords[0] = 0;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagLine,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 2, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagTriangle,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 3, worklet);
  pcoords[0] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[1] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagPolygon,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints > 0, worklet);
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(
            vtkm::CellShapeTagVertex(), numPoints, pcoords, worklet);
      break;
    case 2:
      ParametricCoordinatesCenter(
            vtkm::CellShapeTagLine(), numPoints, pcoords, worklet);
      break;
    case 3:
      ParametricCoordinatesCenter(
            vtkm::CellShapeTagTriangle(), numPoints, pcoords, worklet);
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
void ParametricCoordinatesCenter(vtkm::CellShapeTagPixel,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagQuad,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagTetra,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);
  pcoords[0] = 0.25;
  pcoords[1] = 0.25;
  pcoords[2] = 0.25;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagVoxel,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 8, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0.5;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagHexahedron,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 8, worklet);
  pcoords[0] = 0.5;
  pcoords[1] = 0.5;
  pcoords[2] = 0.5;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagWedge,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 6, worklet);
  pcoords[0] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[1] = static_cast<ParametricCoordType>(1.0/3.0);
  pcoords[2] = 0.5;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesCenter(vtkm::CellShapeTagPyramid,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
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
void ParametricCoordinatesCenter(vtkm::CellShapeTagGeneric shape,
                                 vtkm::IdComponent numPoints,
                                 vtkm::Vec<ParametricCoordType,3> &pcoords,
                                 const vtkm::exec::FunctorBase &worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(ParametricCoordinatesCenter(CellShapeTag(),
                                                          numPoints,
                                                          pcoords,
                                                          worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesCenter.");
  }
}

/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template<typename CellShapeTag>
VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
ParametricCoordinatesCenter(CellShapeTag shape,
                            vtkm::IdComponent numPoints,
                            const vtkm::exec::FunctorBase &worklet)
{
  vtkm::Vec<vtkm::FloatDefault,3> pcoords;
  ParametricCoordinatesCenter(shape, numPoints, pcoords, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagEmpty,
                                vtkm::IdComponent,
                                vtkm::IdComponent,
                                vtkm::Vec<ParametricCoordType,3> &,
                                const vtkm::exec::FunctorBase &worklet)
{
  worklet.RaiseError("Empty cell has no points.");
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagVertex,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
void ParametricCoordinatesPoint(vtkm::CellShapeTagLine,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
void ParametricCoordinatesPoint(vtkm::CellShapeTagTriangle,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 3, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; break;
    case 2: pcoords[0] = 0; pcoords[1] = 1; break;
    default: worklet.RaiseError("Bad point index.");
  }
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagPolygon,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints > 0, worklet);

  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(vtkm::CellShapeTagVertex(),
                                 numPoints,
                                 pointIndex,
                                 pcoords,
                                 worklet);
      return;
    case 2:
      ParametricCoordinatesPoint(vtkm::CellShapeTagLine(),
                                 numPoints,
                                 pointIndex,
                                 pcoords,
                                 worklet);
      return;
    case 3:
      ParametricCoordinatesPoint(vtkm::CellShapeTagTriangle(),
                                 numPoints,
                                 pointIndex,
                                 pcoords,
                                 worklet);
      return;
    case 4:
      ParametricCoordinatesPoint(vtkm::CellShapeTagQuad(),
                                 numPoints,
                                 pointIndex,
                                 pcoords,
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
void ParametricCoordinatesPoint(vtkm::CellShapeTagPixel,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 4, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; break;
    case 2: pcoords[0] = 0; pcoords[1] = 1; break;
    case 3: pcoords[0] = 1; pcoords[1] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
  }
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagQuad,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
  }
  pcoords[2] = 0;
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagTetra,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagVoxel,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(numPoints == 8, worklet);

  switch (pointIndex)
  {
    case 0: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 0; break;
    case 1: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 0; break;
    case 2: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 0; break;
    case 3: pcoords[0] = 1; pcoords[1] = 1; pcoords[2] = 0; break;
    case 4: pcoords[0] = 0; pcoords[1] = 0; pcoords[2] = 1; break;
    case 5: pcoords[0] = 1; pcoords[1] = 0; pcoords[2] = 1; break;
    case 6: pcoords[0] = 0; pcoords[1] = 1; pcoords[2] = 1; break;
    case 7: pcoords[0] = 1; pcoords[1] = 1; pcoords[2] = 1; break;
    default:
      worklet.RaiseError("Bad point index.");
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagHexahedron,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagWedge,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
  }
}

template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagPyramid,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
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
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template<typename ParametricCoordType>
VTKM_EXEC_EXPORT
void ParametricCoordinatesPoint(vtkm::CellShapeTagGeneric shape,
                                vtkm::IdComponent numPoints,
                                vtkm::IdComponent pointIndex,
                                vtkm::Vec<ParametricCoordType,3> &pcoords,
                                const vtkm::exec::FunctorBase &worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(ParametricCoordinatesPoint(CellShapeTag(),
                                                         numPoints,
                                                         pointIndex,
                                                         pcoords,
                                                         worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
  }
}

/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template<typename CellShapeTag>
VTKM_EXEC_EXPORT
vtkm::Vec<vtkm::FloatDefault,3>
ParametricCoordinatesPoint(CellShapeTag shape,
                           vtkm::IdComponent numPoints,
                           vtkm::IdComponent pointIndex,
                           const vtkm::exec::FunctorBase &worklet)
{
  vtkm::Vec<vtkm::FloatDefault,3> pcoords;
  ParametricCoordinatesPoint(shape, numPoints, pointIndex, pcoords, worklet);
  return pcoords;
}

}
} // namespace vtkm::exec

#endif //vtk_m_exec_ParametricCoordinates_h
