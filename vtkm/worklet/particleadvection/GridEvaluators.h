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
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/worklet/particleadvection/CellInterpolationHelper.h>
#include <vtkm/worklet/particleadvection/Field.h>
#include <vtkm/worklet/particleadvection/GridEvaluatorStatus.h>
#include <vtkm/worklet/particleadvection/IntegratorBase.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename DeviceAdapter, typename FieldType>
class ExecutionGridEvaluator
{
  using GhostCellArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8>;

public:
  VTKM_CONT
  ExecutionGridEvaluator() = default;

  VTKM_CONT
  ExecutionGridEvaluator(std::shared_ptr<vtkm::cont::CellLocator> locator,
                         std::shared_ptr<vtkm::cont::CellInterpolationHelper> interpolationHelper,
                         const vtkm::Bounds& bounds,
                         const FieldType& field,
                         const GhostCellArrayType& ghostCells,
                         vtkm::cont::Token& token)
    : Bounds(bounds)
    , Field(field.PrepareForExecution(DeviceAdapter(), token))
    , GhostCells(ghostCells.PrepareForInput(DeviceAdapter(), token))
    , HaveGhostCells(ghostCells.GetNumberOfValues() > 0)
    , InterpolationHelper(interpolationHelper->PrepareForExecution(DeviceAdapter(), token))
    , Locator(locator->PrepareForExecution(DeviceAdapter(), token))
  {
  }

  template <typename Point>
  VTKM_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    vtkm::Id cellId;
    Point parametric;

    Locator->FindCell(point, cellId, parametric);

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
    vtkm::Id cellId;
    Point parametric;
    GridEvaluatorStatus status;

    status.SetOk();
    if (!this->IsWithinTemporalBoundary(time))
    {
      status.SetFail();
      status.SetTemporalBounds();
    }

    Locator->FindCell(point, cellId, parametric);
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
      InterpolationHelper->GetCellInfo(cellId, cellShape, nVerts, ptIndices);

      this->Field->GetValue(ptIndices, nVerts, parametric, cellShape, out);
      status.SetOk();
    }

    return status;
  }

private:
  VTKM_EXEC bool InGhostCell(const vtkm::Id& cellId) const
  {
    if (this->HaveGhostCells && cellId != -1)
      return GhostCells.Get(cellId) == vtkm::CellClassification::GHOST;

    return false;
  }

  using GhostCellPortal = typename vtkm::cont::ArrayHandle<vtkm::UInt8>::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

  vtkm::Bounds Bounds;
  const vtkm::worklet::particleadvection::ExecutionField* Field;
  GhostCellPortal GhostCells;
  bool HaveGhostCells;
  const vtkm::exec::CellInterpolationHelper* InterpolationHelper;
  const vtkm::exec::CellLocator* Locator;
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
      if (arr.IsType<GhostCellArrayType>())
        this->GhostCellArray = arr.Cast<GhostCellArrayType>();
      else
        throw vtkm::cont::ErrorInternal("vtkmGhostCells not of type vtkm::UInt8");
    }
  }

  VTKM_CONT
  GridEvaluator(const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::DynamicCellSet& cellset,
                const FieldType& field)
    : Bounds(coordinates.GetBounds())
    , Field(field)
    , GhostCellArray()
  {
    this->InitializeLocator(coordinates, cellset);
  }

  template <typename DeviceAdapter>
  VTKM_CONT ExecutionGridEvaluator<DeviceAdapter, FieldType> PrepareForExecution(
    DeviceAdapter,
    vtkm::cont::Token& token) const
  {
    return ExecutionGridEvaluator<DeviceAdapter, FieldType>(this->Locator,
                                                            this->InterpolationHelper,
                                                            this->Bounds,
                                                            this->Field,
                                                            this->GhostCellArray,
                                                            token);
  }

private:
  VTKM_CONT void InitializeLocator(const vtkm::cont::CoordinateSystem& coordinates,
                                   const vtkm::cont::DynamicCellSet& cellset)
  {
    if (cellset.IsSameType(Structured2DType()) || cellset.IsSameType(Structured3DType()))
    {
      if (coordinates.GetData().IsType<UniformType>())
      {
        vtkm::cont::CellLocatorUniformGrid locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<vtkm::cont::CellLocatorUniformGrid>(locator);
      }
      else if (coordinates.GetData().IsType<RectilinearType>())
      {
        vtkm::cont::CellLocatorRectilinearGrid locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<vtkm::cont::CellLocatorRectilinearGrid>(locator);
      }
      else
      {
        // Default to using an locator for explicit meshes.
        vtkm::cont::CellLocatorTwoLevel locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<vtkm::cont::CellLocatorTwoLevel>(locator);
      }
      vtkm::cont::StructuredCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::StructuredCellInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
    {
      vtkm::cont::CellLocatorTwoLevel locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<vtkm::cont::CellLocatorTwoLevel>(locator);
      vtkm::cont::SingleCellTypeInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::SingleCellTypeInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      vtkm::cont::CellLocatorTwoLevel locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<vtkm::cont::CellLocatorTwoLevel>(locator);
      vtkm::cont::ExplicitCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::ExplicitCellInterpolationHelper>(interpolationHelper);
    }
    else
      throw vtkm::cont::ErrorInternal("Unsupported cellset type.");
  }

  vtkm::Bounds Bounds;
  FieldType Field;
  GhostCellArrayType GhostCellArray;
  std::shared_ptr<vtkm::cont::CellInterpolationHelper> InterpolationHelper;
  std::shared_ptr<vtkm::cont::CellLocator> Locator;
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_GridEvaluators_h
