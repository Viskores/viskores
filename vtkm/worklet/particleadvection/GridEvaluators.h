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
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/worklet/particleadvection/CellInterpolationHelper.h>
#include <vtkm/worklet/particleadvection/Integrators.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{
class EvaluatorStatus : public vtkm::Bitset<vtkm::UInt8>
{
public:
  VTKM_EXEC_CONT EvaluatorStatus(){};
  VTKM_EXEC_CONT EvaluatorStatus(bool ok, bool spatial, bool temporal)
  {
    this->set(SUCCESS_BIT, ok);
    this->set(SPATIAL_BOUNDS_BIT, spatial);
    this->set(TEMPORAL_BOUNDS_BIT, temporal);
  };
  VTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetFail() { this->reset(SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckFail() const { return !this->test(SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetSpatialBounds() { this->set(SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(SPATIAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTemporalBounds() { this->set(TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(TEMPORAL_BOUNDS_BIT); }

private:
  static constexpr vtkm::Id SUCCESS_BIT = 0;
  static constexpr vtkm::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr vtkm::Id TEMPORAL_BOUNDS_BIT = 2;
};

template <typename DeviceAdapter, typename FieldArrayType>
class ExecutionGridEvaluator
{
  using FieldPortalType =
    typename FieldArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  VTKM_CONT
  ExecutionGridEvaluator() = default;

  VTKM_CONT
  ExecutionGridEvaluator(std::shared_ptr<vtkm::cont::CellLocator> locator,
                         std::shared_ptr<vtkm::cont::CellInterpolationHelper> interpolationHelper,
                         const vtkm::Bounds& bounds,
                         const FieldArrayType& field)
    : Bounds(bounds)
    , Field(field.PrepareForInput(DeviceAdapter()))
  {
    Locator = locator->PrepareForExecution(DeviceAdapter());
    InterpolationHelper = interpolationHelper->PrepareForExecution(DeviceAdapter());
  }

  template <typename Point>
  VTKM_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    vtkm::Id cellId;
    Point parametric;
    vtkm::exec::FunctorBase tmp;

    Locator->FindCell(point, cellId, parametric, tmp);
    return cellId != -1;
  }

  VTKM_EXEC
  bool IsWithinTemporalBoundary(const vtkm::FloatDefault vtkmNotUsed(time)) const { return true; }

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
  VTKM_EXEC EvaluatorStatus Evaluate(const Point& pos,
                                     vtkm::FloatDefault vtkmNotUsed(time),
                                     Point& out) const
  {
    return this->Evaluate(pos, out);
  }

  template <typename Point>
  VTKM_EXEC EvaluatorStatus Evaluate(const Point& point, Point& out) const
  {
    vtkm::Id cellId;
    Point parametric;
    vtkm::exec::FunctorBase tmp;
    EvaluatorStatus status;

    Locator->FindCell(point, cellId, parametric, tmp);
    if (cellId == -1)
    {
      status.SetFail();
      status.SetSpatialBounds();
      return status;
    }

    vtkm::UInt8 cellShape;
    vtkm::IdComponent nVerts;
    vtkm::VecVariable<vtkm::Id, 8> ptIndices;
    vtkm::VecVariable<vtkm::Vec3f, 8> fieldValues;
    InterpolationHelper->GetCellInfo(cellId, cellShape, nVerts, ptIndices);

    for (vtkm::IdComponent i = 0; i < nVerts; i++)
      fieldValues.Append(Field.Get(ptIndices[i]));
    out = vtkm::exec::CellInterpolate(fieldValues, parametric, cellShape, tmp);

    status.SetOk();
    return status;
  }

private:
  const vtkm::exec::CellLocator* Locator;
  const vtkm::exec::CellInterpolationHelper* InterpolationHelper;
  vtkm::Bounds Bounds;
  FieldPortalType Field;
};

template <typename FieldArrayType>
class GridEvaluator : public vtkm::cont::ExecutionObjectBase
{
public:
  using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  VTKM_CONT
  GridEvaluator() = default;

  VTKM_CONT
  GridEvaluator(const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::DynamicCellSet& cellset,
                const FieldArrayType& field)
    : Vectors(field)
    , Bounds(coordinates.GetBounds())
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
        vtkm::cont::CellLocatorUniformBins locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<vtkm::cont::CellLocatorUniformBins>(locator);
      }
      vtkm::cont::StructuredCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::StructuredCellInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
    {
      vtkm::cont::CellLocatorUniformBins locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<vtkm::cont::CellLocatorUniformBins>(locator);
      vtkm::cont::SingleCellTypeInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::SingleCellTypeInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      vtkm::cont::CellLocatorUniformBins locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<vtkm::cont::CellLocatorUniformBins>(locator);
      vtkm::cont::ExplicitCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::ExplicitCellInterpolationHelper>(interpolationHelper);
    }
    else
      throw vtkm::cont::ErrorInternal("Unsupported cellset type.");
  }

  template <typename DeviceAdapter>
  VTKM_CONT ExecutionGridEvaluator<DeviceAdapter, FieldArrayType> PrepareForExecution(
    DeviceAdapter) const
  {
    return ExecutionGridEvaluator<DeviceAdapter, FieldArrayType>(
      this->Locator, this->InterpolationHelper, this->Bounds, this->Vectors);
  }

private:
  std::shared_ptr<vtkm::cont::CellLocator> Locator;
  std::shared_ptr<vtkm::cont::CellInterpolationHelper> InterpolationHelper;
  FieldArrayType Vectors;
  vtkm::Bounds Bounds;
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_GridEvaluators_h
