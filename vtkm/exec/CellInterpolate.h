//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_Interpolate_h
#define vtk_m_exec_Interpolate_h

#include <vtkm/CellShape.h>
#include <vtkm/ErrorCode.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/exec/FunctorBase.h>

#include <lcl/lcl.h>

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

template <typename VtkcCellShapeTag, typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolateImpl(VtkcCellShapeTag tag,
                                              const FieldVecType& field,
                                              const ParametricCoordType& pcoords,
                                              typename FieldVecType::ComponentType& result)
{
  if (tag.numberOfPoints() != field.GetNumberOfComponents())
  {
    result = { 0 };
    return vtkm::ErrorCode::InvalidNumberOfPoints;
  }

  using FieldValueType = typename FieldVecType::ComponentType;
  IdComponent numComponents = vtkm::VecTraits<FieldValueType>::GetNumberOfComponents(field[0]);
  auto status =
    lcl::interpolate(tag, lcl::makeFieldAccessorNestedSOA(field, numComponents), pcoords, result);
  return vtkm::internal::LclErrorToVtkmError(status);
}

} // namespace internal

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType, typename CellShapeTag>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const FieldVecType& pointFieldValues,
                                          const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                          CellShapeTag tag,
                                          typename FieldVecType::ComponentType& result)
{
  auto lclTag = vtkm::internal::make_LclCellShapeTag(tag, pointFieldValues.GetNumberOfComponents());
  return internal::CellInterpolateImpl(lclTag, pointFieldValues, pcoords, result);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const FieldVecType&,
                                          const vtkm::Vec<ParametricCoordType, 3>&,
                                          vtkm::CellShapeTagEmpty,
                                          typename FieldVecType::ComponentType& result)
{
  result = { 0 };
  return vtkm::ErrorCode::OperationOnEmptyCell;
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const FieldVecType& field,
                                          const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                          vtkm::CellShapeTagPolyLine,
                                          typename FieldVecType::ComponentType& result)
{
  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  if (numPoints < 1)
  {
    result = { 0 };
    return vtkm::ErrorCode::InvalidNumberOfPoints;
  }

  if (numPoints == 1)
  {
    return CellInterpolate(field, pcoords, vtkm::CellShapeTagVertex(), result);
  }

  using T = ParametricCoordType;

  T dt = 1 / static_cast<T>(numPoints - 1);
  vtkm::IdComponent idx = static_cast<vtkm::IdComponent>(pcoords[0] / dt);
  if (idx == numPoints - 1)
  {
    result = field[numPoints - 1];
    return vtkm::ErrorCode::Success;
  }

  T pc = (pcoords[0] - static_cast<T>(idx) * dt) / dt;
  return internal::CellInterpolateImpl(
    lcl::Line{}, vtkm::make_Vec(field[idx], field[idx + 1]), &pc, result);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const FieldVecType& field,
                                          const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                          vtkm::CellShapeTagPolygon,
                                          typename FieldVecType::ComponentType& result)
{
  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  if (numPoints < 1)
  {
    result = { 0 };
    return vtkm::ErrorCode::InvalidNumberOfPoints;
  }

  switch (numPoints)
  {
    case 1:
      return CellInterpolate(field, pcoords, vtkm::CellShapeTagVertex(), result);
    case 2:
      return CellInterpolate(field, pcoords, vtkm::CellShapeTagLine(), result);
    default:
      return internal::CellInterpolateImpl(lcl::Polygon(numPoints), field, pcoords, result);
  }
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const vtkm::VecAxisAlignedPointCoordinates<2>& field,
                                          const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                          vtkm::CellShapeTagQuad,
                                          vtkm::Vec3f& result)
{
  return internal::CellInterpolateImpl(lcl::Pixel{}, field, pcoords, result);
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const vtkm::VecAxisAlignedPointCoordinates<3>& field,
                                          const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                          vtkm::CellShapeTagHexahedron,
                                          vtkm::Vec3f& result)
{
  return internal::CellInterpolateImpl(lcl::Voxel{}, field, pcoords, result);
}

//-----------------------------------------------------------------------------
/// \brief Interpolate a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, interpolates the field to that point.
///
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::ErrorCode CellInterpolate(const FieldVecType& pointFieldValues,
                                          const vtkm::Vec<ParametricCoordType, 3>& parametricCoords,
                                          vtkm::CellShapeTagGeneric shape,
                                          typename FieldVecType::ComponentType& result)
{
  vtkm::ErrorCode status;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      status = CellInterpolate(pointFieldValues, parametricCoords, CellShapeTag(), result));
    default:
      result = { 0 };
      status = vtkm::ErrorCode::InvalidShapeId;
  }
  return status;
}

}
} // namespace vtkm::exec

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

#endif //vtk_m_exec_Interpolate_h
