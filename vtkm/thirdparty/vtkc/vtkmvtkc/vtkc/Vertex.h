//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_c_Vertex_h
#define vtk_c_Vertex_h

#include <vtkc/ErrorCode.h>
#include <vtkc/Shapes.h>

namespace vtkc
{

class Vertex : public Cell
{
public:
  constexpr VTKC_EXEC Vertex() : Cell(ShapeId::VERTEX, 1) {}
  constexpr VTKC_EXEC explicit Vertex(const Cell& cell) : Cell(cell) {}
};

VTKC_EXEC inline vtkc::ErrorCode validate(Vertex tag) noexcept
{
  if (tag.shape() != ShapeId::VERTEX)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 1)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricCenter(Vertex, CoordType&&) noexcept
{
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricPoint(Vertex, IdComponent pointId, CoordType&&) noexcept
{
  if (pointId == 0)
  {
    return ErrorCode::SUCCESS;
  }
  else
  {
    return ErrorCode::INVALID_POINT_ID;
  }
}

template<typename CoordType>
VTKC_EXEC inline ComponentType<CoordType> parametricDistance(Vertex, const CoordType&) noexcept
{
  return ComponentType<CoordType>{1};
}

template<typename CoordType>
VTKC_EXEC inline bool cellInside(Vertex, const CoordType&) noexcept
{
  return false;
}

template <typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode interpolate(
  Vertex,
  const Values& values,
  const CoordType&,
  Result&& result) noexcept
{
  using T = ComponentType<Result>;
  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    component(result, c) = static_cast<T>(values.getValue(0, c));
  }

  return ErrorCode::SUCCESS;
}

template <typename Points, typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode derivative(
  Vertex,
  const Points&,
  const Values& values,
  const CoordType&,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  using T = ComponentType<Result>;
  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    component(dx, c) = component(dy, c) = component(dz, c) = T{0};
  }
  return ErrorCode::SUCCESS;
}

template <typename Points, typename PCoordType, typename WCoordType>
VTKC_EXEC inline  vtkc::ErrorCode parametricToWorld(
  Vertex,
  const Points& points,
  const PCoordType&,
  WCoordType&& wcoords) noexcept
{
  using T = ComponentType<WCoordType>;
  component(wcoords, 0) = static_cast<T>(points.getValue(0, 0));
  component(wcoords, 1) = static_cast<T>(points.getValue(0, 1));
  component(wcoords, 2) = static_cast<T>(points.getValue(0, 2));
  return ErrorCode::SUCCESS;
}

template <typename Points, typename WCoordType, typename PCoordType>
VTKC_EXEC inline vtkc::ErrorCode worldToParametric(
  Vertex,
  const Points&,
  const WCoordType&,
  PCoordType&&) noexcept
{
  return ErrorCode::SUCCESS;
}

} //namespace vtkc

#endif //vtk_c_Vertex_h
