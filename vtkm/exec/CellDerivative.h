//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_Derivative_h
#define vtk_m_exec_Derivative_h

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/VecTraits.h>

#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>

#include <lcl/lcl.h>

namespace vtkm
{
namespace exec
{

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
namespace internal
{

template <typename VtkcCellShapeTag,
          typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivativeImpl(
  VtkcCellShapeTag tag,
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const ParametricCoordType& pcoords,
  const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSERT(field.GetNumberOfComponents() == tag.numberOfPoints());
  VTKM_ASSERT(wCoords.GetNumberOfComponents() == tag.numberOfPoints());

  using FieldType = typename FieldVecType::ComponentType;

  auto fieldNumComponents = vtkm::VecTraits<FieldType>::GetNumberOfComponents(field[0]);
  vtkm::Vec<FieldType, 3> derivs;
  auto status = lcl::derivative(tag,
                                lcl::makeFieldAccessorNestedSOA(wCoords, 3),
                                lcl::makeFieldAccessorNestedSOA(field, fieldNumComponents),
                                pcoords,
                                derivs[0],
                                derivs[1],
                                derivs[2]);
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
    derivs = vtkm::TypeTraits<vtkm::Vec<FieldType, 3>>::ZeroInitialization();
  }

  return derivs;
}

} // namespace internal

template <typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType,
          typename CellShapeTag>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag shape,
  const vtkm::exec::FunctorBase& worklet)
{
  return internal::CellDerivativeImpl(
    vtkm::internal::make_VtkcCellShapeTag(shape), field, wCoords, pcoords, worklet);
}

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

template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagPolyLine,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  VTKM_ASSERT(numPoints >= 1);
  VTKM_ASSERT(numPoints == wCoords.GetNumberOfComponents());

  switch (numPoints)
  {
    case 1:
      return CellDerivative(field, wCoords, pcoords, vtkm::CellShapeTagVertex(), worklet);
    case 2:
      return CellDerivative(field, wCoords, pcoords, vtkm::CellShapeTagLine(), worklet);
  }

  auto dt = static_cast<ParametricCoordType>(1) / static_cast<ParametricCoordType>(numPoints - 1);
  auto idx = static_cast<vtkm::IdComponent>(vtkm::Ceil(pcoords[0] / dt));
  if (idx == 0)
  {
    idx = 1;
  }
  if (idx > numPoints - 1)
  {
    idx = numPoints - 1;
  }

  auto lineField = vtkm::make_Vec(field[idx - 1], field[idx]);
  auto lineWCoords = vtkm::make_Vec(wCoords[idx - 1], wCoords[idx]);
  auto pc = (pcoords[0] - static_cast<ParametricCoordType>(idx) * dt) / dt;
  return internal::CellDerivativeImpl(lcl::Line{}, lineField, lineWCoords, &pc, worklet);
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
    default:
      return internal::CellDerivativeImpl(
        lcl::Polygon(numPoints), field, wCoords, pcoords, worklet);
  }
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const vtkm::VecAxisAlignedPointCoordinates<2>& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagQuad,
  const vtkm::exec::FunctorBase& worklet)
{
  return internal::CellDerivativeImpl(lcl::Pixel{}, field, wCoords, pcoords, worklet);
}

template <typename FieldVecType, typename ParametricCoordType>
VTKM_EXEC vtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const vtkm::VecAxisAlignedPointCoordinates<3>& wCoords,
  const vtkm::Vec<ParametricCoordType, 3>& pcoords,
  vtkm::CellShapeTagHexahedron,
  const vtkm::exec::FunctorBase& worklet)
{
  return internal::CellDerivativeImpl(lcl::Voxel{}, field, wCoords, pcoords, worklet);
}
}
} // namespace vtkm::exec

#endif //vtk_m_exec_Derivative_h
