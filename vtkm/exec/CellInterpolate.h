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

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
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
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolateImpl(
  VtkcCellShapeTag tag,
  const FieldVecType& field,
  const ParametricCoordType& pcoords,
  const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSERT(tag.numberOfPoints() == field.GetNumberOfComponents());

  using FieldValueType = typename FieldVecType::ComponentType;
  IdComponent numComponents = vtkm::VecTraits<FieldValueType>::GetNumberOfComponents(field[0]);
  FieldValueType result(0);
  auto status =
    lcl::interpolate(tag, lcl::makeFieldAccessorNestedSOA(field, numComponents), pcoords, result);
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
  }
  return result;
}

} // namespace internal

//-----------------------------------------------------------------------------
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
template <typename FieldVecType, typename ParametricCoordType, typename CellShapeTag>
VTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& pointFieldValues,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag tag,
  const vtkm::exec::FunctorBase& worklet)
{
  auto lclTag =
    vtkm::internal::make_VtkcCellShapeTag(tag, pointFieldValues.GetNumberOfComponents());
  return internal::CellInterpolateImpl(lclTag, pointFieldValues, pcoords, worklet);
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
  const FieldVecType& field,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPolyLine,
  const vtkm::exec::FunctorBase& worklet)
{
  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  VTKM_ASSERT(numPoints >= 1);

  if (numPoints == 1)
  {
    return CellInterpolate(field, pcoords, vtkm::CellShapeTagVertex(), worklet);
  }

  using T = ParametricCoordType;

  T dt = 1 / static_cast<T>(numPoints - 1);
  vtkm::IdComponent idx = static_cast<vtkm::IdComponent>(pcoords[0] / dt);
  if (idx == numPoints - 1)
  {
    return field[numPoints - 1];
  }

  T pc = (pcoords[0] - static_cast<T>(idx) * dt) / dt;
  return internal::CellInterpolateImpl(
    lcl::Line{}, vtkm::make_Vec(field[idx], field[idx + 1]), &pc, worklet);
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
    default:
      return internal::CellInterpolateImpl(lcl::Polygon(numPoints), field, pcoords, worklet);
  }
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VTKM_EXEC vtkm::Vec3f CellInterpolate(const vtkm::VecAxisAlignedPointCoordinates<2>& field,
                                      const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                      vtkm::CellShapeTagQuad,
                                      const vtkm::exec::FunctorBase& worklet)
{
  return internal::CellInterpolateImpl(lcl::Pixel{}, field, pcoords, worklet);
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VTKM_EXEC vtkm::Vec3f CellInterpolate(const vtkm::VecAxisAlignedPointCoordinates<3>& field,
                                      const vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                      vtkm::CellShapeTagHexahedron,
                                      const vtkm::exec::FunctorBase& worklet)
{
  return internal::CellInterpolateImpl(lcl::Voxel{}, field, pcoords, worklet);
}
}
} // namespace vtkm::exec

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

#endif //vtk_m_exec_Interpolate_h
