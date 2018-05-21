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
#ifndef vtk_m_exec_Interpolate_h
#define vtk_m_exec_Interpolate_h

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/exec/FunctorBase.h>

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

namespace vtkm
{
namespace exec
{

namespace internal
{

// This is really the WorldCoorindatesToParametericCoordinates function, but
// moved to this header file because it is required to interpolate in a
// polygon, which is divided into triangles.
template <typename WorldCoordVector>
VTKM_EXEC typename WorldCoordVector::ComponentType ReverseInterpolateTriangle(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords)
{
  VTKM_ASSERT(pointWCoords.GetNumberOfComponents() == 3);

  // We will solve the world to parametric coordinates problem geometrically.
  // Consider the parallelogram formed by wcoords and p0 of the triangle and
  // the two adjacent edges. This parallelogram is equivalent to the
  // axis-aligned rectangle anchored at the origin of parametric space.
  //
  //   p2 |\                 (1,0) |\                                        //
  //      | \                      |  \                                      //
  //      |  \                     |    \                                    //
  //     |    \                    |      \                                  //
  //     |     \                   |        \                                //
  //     |      \                  |    (u,v) \                              //
  //    | ---    \                 |-------*    \                            //
  //    |    ---*wcoords           |       |      \                          //
  //    |       |  \               |       |        \                        //
  // p0 *---    |   \        (0,0) *------------------\ (1,0)                //
  //        ---|     \                                                       //
  //           x--    \                                                      //
  //              ---  \                                                     //
  //                 ---\ p1                                                 //
  //
  // In this diagram, the distance between p0 and the point marked x divided by
  // the length of the edge it is on is equal, by proportionality, to the u
  // parametric coordinate. (The v coordinate follows the other edge
  // accordingly.) Thus, if we can find the intersection at x (or more
  // specifically the distance between p0 and x), then we can find that
  // parametric coordinate.
  //
  // Because the triangle is in 3-space, we are actually going to intersect the
  // edge with a plane that is parallel to the opposite edge of p0 and
  // perpendicular to the triangle. This is partially because it is easy to
  // find the intersection between a plane and a line and partially because the
  // computation will work for points not on the plane. (The result is
  // equivalent to a point projected on the plane.)
  //
  // First, we define an implicit plane as:
  //
  // Dot((p - wcoords), planeNormal) = 0
  //
  // where planeNormal is the normal to the plane (easily computed from the
  // triangle), and p is any point in the plane. Next, we define the parametric
  // form of the line:
  //
  // p(d) = (p1 - p0)d + p0
  //
  // Where d is the fraction of distance from p0 toward p1. Note that d is
  // actually equal to the parametric coordinate we are trying to find. Once we
  // compute it, we are done. We can skip the part about finding the actual
  // coordinates of the intersection.
  //
  // Solving for the intersection is as simple as substituting the line's
  // definition of p(d) into p for the plane equation. With some basic algebra
  // you get:
  //
  // d = Dot((wcoords - p0), planeNormal)/Dot((p1-p0), planeNormal)
  //
  // From here, the u coordinate is simply d. The v coordinate follows
  // similarly.
  //

  using Vector3 = typename WorldCoordVector::ComponentType;
  using T = typename Vector3::ComponentType;

  Vector3 pcoords(T(0));
  Vector3 triangleNormal = vtkm::TriangleNormal(pointWCoords[0], pointWCoords[1], pointWCoords[2]);

  for (vtkm::IdComponent dimension = 0; dimension < 2; dimension++)
  {
    Vector3 p0 = pointWCoords[0];
    Vector3 p1 = pointWCoords[dimension + 1];
    Vector3 p2 = pointWCoords[2 - dimension];
    Vector3 planeNormal = vtkm::Cross(triangleNormal, p2 - p0);

    T d = vtkm::Dot(wcoords - p0, planeNormal) / vtkm::Dot(p1 - p0, planeNormal);

    pcoords[dimension] = d;
  }

  return pcoords;
}
}

/// \brief Interpolate a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, interpolates the field to that point.
///
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& pointFieldValues,
  const vtkm::Vec<ParametricCoordType, 3>& parametricCoords,
  vtkm::CellShapeTagGeneric shape,
  const vtkm::exec::FunctorBase& worklet)
{
  typename FieldVecType::ComponentType result;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      result = CellInterpolate(pointFieldValues, parametricCoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Unknown cell shape sent to interpolate.");
      return typename FieldVecType::ComponentType();
  }
  return result;
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType&,
  const vtkm::Vec<ParametricCoordType, 3>&,
  vtkm::CellShapeTagEmpty,
  const vtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to interpolate an empty cell.");
  return typename FieldVecType::ComponentType();
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& pointFieldValues,
  const vtkm::Vec<ParametricCoordType, 3>,
  vtkm::CellShapeTagVertex,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(pointFieldValues.GetNumberOfComponents() == 1);
  return pointFieldValues[0];
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& pointFieldValues,
  const vtkm::Vec<ParametricCoordType, 3>& parametricCoords,
  vtkm::CellShapeTagLine,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(pointFieldValues.GetNumberOfComponents() == 2);
  return vtkm::Lerp(pointFieldValues[0], pointFieldValues[1], parametricCoords[0]);
}

template <typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> CellInterpolate(
  const vtkm::VecAxisAlignedPointCoordinates<1>& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagLine,
  const vtkm::exec::FunctorBase&)
{
  using T = vtkm::Vec<vtkm::FloatDefault, 3>;

  const T& origin = field.GetOrigin();
  const T& spacing = field.GetSpacing();

  return T(
    origin[0] + static_cast<vtkm::FloatDefault>(pcoords[0]) * spacing[0], origin[1], origin[2]);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagTriangle,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 3);
  using T = typename FieldVecType::ComponentType;
  return static_cast<T>((field[0] * (1 - pcoords[0] - pcoords[1])) + (field[1] * pcoords[0]) +
                        (field[2] * pcoords[1]));
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPolygon,
  const vtkm::exec::FunctorBase& worklet)
{
  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  VTKM_ASSERT(numPoints > 0);
  switch (numPoints)
  {
    case 1:
      return CellInterpolate(field, pcoords, vtkm::CellShapeTagVertex(), worklet);
    case 2:
      return CellInterpolate(field, pcoords, vtkm::CellShapeTagLine(), worklet);
    case 3:
      return CellInterpolate(field, pcoords, vtkm::CellShapeTagTriangle(), worklet);
    case 4:
      return CellInterpolate(field, pcoords, vtkm::CellShapeTagQuad(), worklet);
  }

  // If we are here, then there are 5 or more points on this polygon.

  // Arrange the points such that they are on the circle circumscribed in the
  // unit square from 0 to 1. That is, the point are on the circle centered at
  // coordinate 0.5,0.5 with radius 0.5. The polygon is divided into regions
  // defined by they triangle fan formed by the points around the center. This
  // is C0 continuous but not necessarily C1 continuous. It is also possible to
  // have a non 1 to 1 mapping between parametric coordinates world coordinates
  // if the polygon is not planar or convex.

  using FieldType = typename FieldVecType::ComponentType;

  // Find the interpolation for the center point.
  FieldType fieldCenter = field[0];
  for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
  {
    fieldCenter = fieldCenter + field[pointIndex];
  }
  fieldCenter = fieldCenter * FieldType(1.0f / static_cast<float>(numPoints));

  if ((vtkm::Abs(pcoords[0] - 0.5f) < 4 * vtkm::Epsilon<ParametricCoordType>()) &&
      (vtkm::Abs(pcoords[1] - 0.5f) < 4 * vtkm::Epsilon<ParametricCoordType>()))
  {
    return fieldCenter;
  }

  ParametricCoordType angle = vtkm::ATan2(pcoords[1] - 0.5f, pcoords[0] - 0.5f);
  if (angle < 0)
  {
    angle += 2 * vtkm::Pi<ParametricCoordType>();
  }
  const ParametricCoordType deltaAngle = 2 * vtkm::Pi<ParametricCoordType>() / numPoints;
  vtkm::IdComponent firstPointIndex =
    static_cast<vtkm::IdComponent>(vtkm::Floor(angle / deltaAngle));
  vtkm::IdComponent secondPointIndex = firstPointIndex + 1;
  if (secondPointIndex == numPoints)
  {
    secondPointIndex = 0;
  }

  // Transform pcoords for polygon into pcoords for triangle.
  vtkm::Vec<vtkm::Vec<ParametricCoordType, 3>, 3> polygonCoords;
  polygonCoords[0][0] = 0.5f;
  polygonCoords[0][1] = 0.5f;
  polygonCoords[0][2] = 0;

  polygonCoords[1][0] =
    0.5f * (vtkm::Cos(deltaAngle * static_cast<ParametricCoordType>(firstPointIndex)) + 1);
  polygonCoords[1][1] =
    0.5f * (vtkm::Sin(deltaAngle * static_cast<ParametricCoordType>(firstPointIndex)) + 1);
  polygonCoords[1][2] = 0.0f;

  polygonCoords[2][0] =
    0.5f * (vtkm::Cos(deltaAngle * static_cast<ParametricCoordType>(secondPointIndex)) + 1);
  polygonCoords[2][1] =
    0.5f * (vtkm::Sin(deltaAngle * static_cast<ParametricCoordType>(secondPointIndex)) + 1);
  polygonCoords[2][2] = 0.0f;

  vtkm::Vec<ParametricCoordType, 3> trianglePCoords =
    vtkm::exec::internal::ReverseInterpolateTriangle(polygonCoords, pcoords);

  // Set up parameters for triangle that pcoords is in
  vtkm::Vec<FieldType, 3> triangleField;
  triangleField[0] = fieldCenter;
  triangleField[1] = field[firstPointIndex];
  triangleField[2] = field[secondPointIndex];

  // Now use the triangle interpolate
  return vtkm::exec::CellInterpolate(
    triangleField, trianglePCoords, vtkm::CellShapeTagTriangle(), worklet);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagQuad,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 4);

  using T = typename FieldVecType::ComponentType;

  T bottomInterp = vtkm::Lerp(field[0], field[1], pcoords[0]);
  T topInterp = vtkm::Lerp(field[3], field[2], pcoords[0]);

  return vtkm::Lerp(bottomInterp, topInterp, pcoords[1]);
}

template <typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> CellInterpolate(
  const vtkm::VecAxisAlignedPointCoordinates<2>& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagQuad,
  const vtkm::exec::FunctorBase&)
{
  using T = vtkm::Vec<vtkm::FloatDefault, 3>;

  const T& origin = field.GetOrigin();
  const T& spacing = field.GetSpacing();

  return T(origin[0] + static_cast<vtkm::FloatDefault>(pcoords[0]) * spacing[0],
           origin[1] + static_cast<vtkm::FloatDefault>(pcoords[1]) * spacing[1],
           origin[2]);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagTetra,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 4);
  using T = typename FieldVecType::ComponentType;
  return static_cast<T>((field[0] * (1 - pcoords[0] - pcoords[1] - pcoords[2])) +
                        (field[1] * pcoords[0]) + (field[2] * pcoords[1]) +
                        (field[3] * pcoords[2]));
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagHexahedron,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 8);

  using T = typename FieldVecType::ComponentType;

  T bottomFrontInterp = vtkm::Lerp(field[0], field[1], pcoords[0]);
  T bottomBackInterp = vtkm::Lerp(field[3], field[2], pcoords[0]);
  T topFrontInterp = vtkm::Lerp(field[4], field[5], pcoords[0]);
  T topBackInterp = vtkm::Lerp(field[7], field[6], pcoords[0]);

  T bottomInterp = vtkm::Lerp(bottomFrontInterp, bottomBackInterp, pcoords[1]);
  T topInterp = vtkm::Lerp(topFrontInterp, topBackInterp, pcoords[1]);

  return vtkm::Lerp(bottomInterp, topInterp, pcoords[2]);
}

template <typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<vtkm::FloatDefault, 3> CellInterpolate(
  const vtkm::VecAxisAlignedPointCoordinates<3>& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagHexahedron,
  const vtkm::exec::FunctorBase&)
{
  vtkm::Vec<vtkm::FloatDefault, 3> pcoordsCast(static_cast<vtkm::FloatDefault>(pcoords[0]),
                                               static_cast<vtkm::FloatDefault>(pcoords[1]),
                                               static_cast<vtkm::FloatDefault>(pcoords[2]));

  return field.GetOrigin() + pcoordsCast * field.GetSpacing();
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagWedge,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 6);

  using T = typename FieldVecType::ComponentType;

  T bottomInterp = static_cast<T>((field[0] * (1 - pcoords[0] - pcoords[1])) +
                                  (field[1] * pcoords[1]) + (field[2] * pcoords[0]));

  T topInterp = static_cast<T>((field[3] * (1 - pcoords[0] - pcoords[1])) +
                               (field[4] * pcoords[1]) + (field[5] * pcoords[0]));

  return vtkm::Lerp(bottomInterp, topInterp, pcoords[2]);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPyramid,
  const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  VTKM_ASSERT(field.GetNumberOfComponents() == 5);

  using T = typename FieldVecType::ComponentType;

  T frontInterp = vtkm::Lerp(field[0], field[1], pcoords[0]);
  T backInterp = vtkm::Lerp(field[3], field[2], pcoords[0]);

  T baseInterp = vtkm::Lerp(frontInterp, backInterp, pcoords[1]);

  return vtkm::Lerp(baseInterp, field[4], pcoords[2]);
}
}
} // namespace vtkm::exec

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

#endif //vtk_m_exec_Interpolate_h
