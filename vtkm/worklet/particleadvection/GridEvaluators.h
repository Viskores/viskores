//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_GridEvaluators_h
#define vtk_m_worklet_particleadvection_GridEvaluators_h

#include <vtkm/Bitset.h>
#include <vtkm/CellClassification.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocatorGeneral.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/worklet/particleadvection/CellInterpolationHelper.h>
#include <vtkm/worklet/particleadvection/Field.h>
#include <vtkm/worklet/particleadvection/GridEvaluatorStatus.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename FieldType>
class ExecutionGridEvaluator
{
  using GhostCellArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8>;

public:
  VTKM_CONT
  ExecutionGridEvaluator() = default;

  VTKM_CONT
  ExecutionGridEvaluator(const vtkm::cont::CellLocatorGeneral& locator,
                         const vtkm::cont::CellInterpolationHelper interpolationHelper,
                         const vtkm::Bounds& bounds,
                         const FieldType& field,
                         const GhostCellArrayType& ghostCells,
                         vtkm::cont::DeviceAdapterId device,
                         vtkm::cont::Token& token)
    : Bounds(bounds)
    , Field(field.PrepareForExecution(device, token))
    , GhostCells(ghostCells.PrepareForInput(device, token))
    , HaveGhostCells(ghostCells.GetNumberOfValues() > 0)
    , InterpolationHelper(interpolationHelper.PrepareForExecution(device, token))
    , Locator(locator.PrepareForExecution(device, token))
  {
  }

  template <typename Point>
  VTKM_EXEC bool IsWithinSpatialBoundary(const Point& point) const
  {
    vtkm::Id cellId = -1;
    Point parametric;

    this->Locator.FindCell(point, cellId, parametric);

    if (cellId == -1)
      return false;
    else
      return !this->InGhostCell(cellId);
  }

  VTKM_EXEC
  bool IsWithinTemporalBoundary(const vtkm::FloatDefault& vtkmNotUsed(time)) const { return true; }

  VTKM_EXEC
  vtkm::Bounds GetSpatialBoundary() const { return this->Bounds; }

  VTKM_EXEC_CONT
  vtkm::FloatDefault GetTemporalBoundary(vtkm::Id direction) const
  {
    // Return the time of the newest time slice
    return direction > 0 ? vtkm::Infinity<vtkm::FloatDefault>()
                         : vtkm::NegativeInfinity<vtkm::FloatDefault>();
  }

  template <typename Point>
  VTKM_EXEC GridEvaluatorStatus Evaluate(const Point& point,
                                         const vtkm::FloatDefault& time,
                                         vtkm::VecVariable<Point, 2>& out) const
  {
    vtkm::Id cellId = -1;
    Point parametric;
    GridEvaluatorStatus status;

    status.SetOk();
    if (!this->IsWithinTemporalBoundary(time))
    {
      status.SetFail();
      status.SetTemporalBounds();
    }

    this->Locator.FindCell(point, cellId, parametric);
    if (cellId == -1)
    {
      status.SetFail();
      status.SetSpatialBounds();
    }
    else if (this->InGhostCell(cellId))
    {
      status.SetFail();
      status.SetInGhostCell();
      status.SetSpatialBounds();
    }

    //If initial checks ok, then do the evaluation.
    if (status.CheckOk())
    {
      vtkm::UInt8 cellShape;
      vtkm::IdComponent nVerts;
      vtkm::VecVariable<vtkm::Id, 8> ptIndices;
      vtkm::VecVariable<vtkm::Vec3f, 8> fieldValues;

      if (this->Field.GetAssociation() == vtkm::cont::Field::Association::Points)
      {
        this->InterpolationHelper.GetCellInfo(cellId, cellShape, nVerts, ptIndices);
        this->Field.GetValue(ptIndices, nVerts, parametric, cellShape, out);
      }
      else if (this->Field.GetAssociation() == vtkm::cont::Field::Association::Cells)
      {
        this->Field.GetValue(cellId, out);
      }

      status.SetOk();
    }

    return status;
  }

private:
  VTKM_EXEC bool InGhostCell(const vtkm::Id& cellId) const
  {
    if (this->HaveGhostCells && cellId != -1)
      return GhostCells.Get(cellId) == vtkm::CellClassification::Ghost;

    return false;
  }

  using GhostCellPortal = typename vtkm::cont::ArrayHandle<vtkm::UInt8>::ReadPortalType;

  vtkm::Bounds Bounds;
  typename FieldType::ExecutionType Field;
  GhostCellPortal GhostCells;
  bool HaveGhostCells;
  vtkm::exec::CellInterpolationHelper InterpolationHelper;
  typename vtkm::cont::CellLocatorGeneral::ExecObjType Locator;
};

template <typename FieldType>
class GridEvaluator : public vtkm::cont::ExecutionObjectBase
{
public:
  using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;
  using GhostCellArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8>;

  VTKM_CONT
  GridEvaluator() = default;

  VTKM_CONT
  GridEvaluator(const vtkm::cont::DataSet& dataSet, const FieldType& field)
    : Bounds(dataSet.GetCoordinateSystem().GetBounds())
    , Field(field)
    , GhostCellArray()
  {
    this->InitializeLocator(dataSet.GetCoordinateSystem(), dataSet.GetCellSet());

    if (dataSet.HasCellField("vtkmGhostCells"))
    {
      auto arr = dataSet.GetCellField("vtkmGhostCells").GetData();
      vtkm::cont::ArrayCopyShallowIfPossible(arr, this->GhostCellArray);
    }
  }

  VTKM_CONT
  GridEvaluator(const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::UnknownCellSet& cellset,
                const FieldType& field)
    : Bounds(coordinates.GetBounds())
    , Field(field)
    , GhostCellArray()
  {
    this->InitializeLocator(coordinates, cellset);
  }

  VTKM_CONT ExecutionGridEvaluator<FieldType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return ExecutionGridEvaluator<FieldType>(this->Locator,
                                             this->InterpolationHelper,
                                             this->Bounds,
                                             this->Field,
                                             this->GhostCellArray,
                                             device,
                                             token);
  }

private:
  VTKM_CONT void InitializeLocator(const vtkm::cont::CoordinateSystem& coordinates,
                                   const vtkm::cont::UnknownCellSet& cellset)
  {
    this->Locator.SetCoordinates(coordinates);
    this->Locator.SetCellSet(cellset);
    this->Locator.Update();
    this->InterpolationHelper = vtkm::cont::CellInterpolationHelper(cellset);
  }

  vtkm::Bounds Bounds;
  FieldType Field;
  GhostCellArrayType GhostCellArray;
  vtkm::cont::CellInterpolationHelper InterpolationHelper;
  vtkm::cont::CellLocatorGeneral Locator;
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_GridEvaluators_h
