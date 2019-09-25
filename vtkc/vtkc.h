//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_c_vtkc_h
#define vtk_c_vtkc_h

#include <vtkc/Hexahedron.h>
#include <vtkc/Line.h>
#include <vtkc/Pixel.h>
#include <vtkc/Polygon.h>
#include <vtkc/Pyramid.h>
#include <vtkc/Quad.h>
#include <vtkc/Tetra.h>
#include <vtkc/Triangle.h>
#include <vtkc/Vertex.h>
#include <vtkc/Voxel.h>
#include <vtkc/Wedge.h>

#include <utility>

namespace vtkc
{

/// \brief Perform basic checks to validate cell's state.
/// \param[in]  tag  The cell tag to validate.
/// \return          vtkc::ErrorCode::SUCCESS if valid.
///
VTKC_EXEC inline vtkc::ErrorCode validate(Cell tag) noexcept
{
  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = validate(CellTag{tag}));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
      break;
  }
  return status;
}

/// \brief Return center of a cell in parametric coordinates.
/// \remark Note that the parametric center is not always located at (0.5,0.5,0.5).
/// \param[in]   tag      The cell tag.
/// \param[out]  pcoords  The center of the cell in parametric coordinates.
/// \return               A vtkc::ErrorCode value indicating the status of the operation.
///
template<typename CoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricCenter(Cell tag, CoordType&& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = parametricCenter(CellTag{tag}, std::forward<CoordType>(pcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
      break;
  }
  return status;
}

/// \brief Return the parametric coordinates of a cell's point.
/// \param[in]   tag      The cell tag.
/// \param[in]   pointId  The point number.
/// \param[out]  pcoords  The parametric coordinates of a cell's point.
/// \return               A vtkc::ErrorCode value indicating the status of the operation.
///
template<typename CoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricPoint(
  Cell tag, IdComponent pointId, CoordType&& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = parametricPoint(CellTag{tag}, pointId, std::forward<CoordType>(pcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
      break;
  }
  return status;
}

/// \brief Return the parametric distance of a parametric coordinate to a cell.
/// \param[in]  tag      The cell tag.
/// \param[in]  pcoords  The parametric coordinates of the point.
/// \return              The parametric distance of the point to the cell.
///                      If point is inside the cell, 0 is returned.
/// \pre tag should be a valid cell, otherwise the result is undefined.
///
template<typename CoordType>
VTKC_EXEC inline ComponentType<CoordType> parametricDistance(Cell tag, const CoordType& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ComponentType<CoordType> dist{0};
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(dist = parametricDistance(CellTag{tag}, pcoords));
    default:
      break;
  }
  return dist;
}

/// \brief Check if the given parametric point lies inside a cell.
/// \param[in]  tag      The cell tag.
/// \param[in]  pcoords  The parametric coordinates of the point.
/// \return              true if inside, false otherwise.
/// \pre tag should be a valid cell, otherwise the result is undefined.
///
template<typename CoordType>
VTKC_EXEC inline bool cellInside(Cell tag, const CoordType& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  bool inside = false;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(inside = cellInside(CellTag{tag}, pcoords));
    default:
      break;
  }
  return inside;
}

/// \brief Interpolate \c values at the paramteric coordinates \c pcoords
/// \param[in]   tag      The cell tag.
/// \param[in]   values   A \c FieldAccessor for values to interpolate
/// \param[in]   pcoords  The parametric coordinates.
/// \param[out]  result   The interpolation result
/// \return               A vtkc::ErrorCode value indicating the status of the operation.
///
template <typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode interpolate(
  Cell tag,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = interpolate(CellTag{tag}, values, pcoords, std::forward<Result>(result)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

/// \brief Compute derivative of \c values at the paramteric coordinates \c pcoords
/// \param[in]   tag      The cell tag.
/// \param[in]   points   A \c FieldAccessor for points of the cell
/// \param[in]   values   A \c FieldAccessor for the values to compute derivative of.
/// \param[in]   pcoords  The parametric coordinates.
/// \param[out]  dx       The derivative along X
/// \param[out]  dy       The derivative along Y
/// \param[out]  dz       The derivative along Z
/// \return               A vtkc::ErrorCode value indicating the status of the operation.
///
template <typename Points, typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode derivative(
  Cell tag,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = derivative(CellTag{tag},
                                                points,
                                                values,
                                                pcoords,
                                                std::forward<Result>(dx),
                                                std::forward<Result>(dy),
                                                std::forward<Result>(dz)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

/// \brief Compute world coordinates from parametric coordinates
/// \param[in]   tag      The cell tag.
/// \param[in]   points   A \c FieldAccessor for points of the cell
/// \param[in]   pcoords  The parametric coordinates.
/// \param[out]  wcoords  The world coordinates.
///
template <typename Points, typename PCoordType, typename WCoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricToWorld(
  Cell tag,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = parametricToWorld(CellTag{tag}, points, pcoords, std::forward<WCoordType>(wcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

/// \brief Compute parametric coordinates from world coordinates
/// \param[in]   tag      The cell tag.
/// \param[in]   points   A \c FieldAccessor for points of the cell
/// \param[in]   wcoords  The world coordinates.
/// \param[out]  pcoords  The parametric coordinates.
///
template <typename Points, typename WCoordType, typename PCoordType>
VTKC_EXEC inline vtkc::ErrorCode worldToParametric(
  Cell tag,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    vtkcGenericCellShapeMacro(status = worldToParametric(CellTag{tag}, points, wcoords, std::forward<PCoordType>(pcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

} //namespace vtkc

#endif //vtk_c_vtkc_h
