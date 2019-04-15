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

#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
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
  void GetSpatialBoundary(vtkm::Vec<vtkm::FloatDefault, 3>& dir,
                          vtkm::Vec<ScalarType, 3>& boundary) const
  {
    // Based on the direction of the velocity we need to be able to tell where
    // the particle will exit the domain from to actually push it out of domain.
    boundary[0] = static_cast<ScalarType>(dir[0] > 0 ? this->Bounds.X.Max : this->Bounds.X.Min);
    boundary[1] = static_cast<ScalarType>(dir[1] > 0 ? this->Bounds.Y.Max : this->Bounds.Y.Min);
    boundary[2] = static_cast<ScalarType>(dir[2] > 0 ? this->Bounds.Z.Max : this->Bounds.Z.Min);
  }
  VTKM_EXEC_CONT
  void GetTemporalBoundary(vtkm::FloatDefault& boundary) const
  {
    // Return the time of the newest time slice
    boundary = 0;
  }

  template <typename Point>
  VTKM_EXEC bool Evaluate(const Point& pos, vtkm::FloatDefault vtkmNotUsed(time), Point& out) const
  {
    return this->Evaluate(pos, out);
  }

  template <typename Point>
  VTKM_EXEC bool Evaluate(const Point point, Point& out) const
  {
    vtkm::Id cellId;
    Point parametric;
    vtkm::exec::FunctorBase tmp;
    Locator->FindCell(point, cellId, parametric, tmp);
    if (cellId == -1)
      return false;

    vtkm::UInt8 cellShape;
    vtkm::IdComponent nVerts;
    vtkm::VecVariable<vtkm::Id, 8> ptIndices;
    vtkm::VecVariable<vtkm::Vec<vtkm::FloatDefault, 3>, 8> fieldValues;
    InterpolationHelper->GetCellInfo(cellId, cellShape, nVerts, ptIndices);

    for (vtkm::IdComponent i = 0; i < nVerts; i++)
      fieldValues.Append(Field.Get(ptIndices[i]));
    out = vtkm::exec::CellInterpolate(fieldValues, parametric, cellShape, tmp);

    return true;
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
  using StructuredType = vtkm::cont::CellSetStructured<3>;

  VTKM_CONT
  GridEvaluator() = default;

  VTKM_CONT
  GridEvaluator(const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::DynamicCellSet& cellset,
                const FieldArrayType& field)
    : Vectors(field)
    , Bounds(coordinates.GetBounds())
  {
    if (cellset.IsSameType(StructuredType()))
    {
      if (coordinates.GetData().IsType<UniformType>())
      {
        vtkm::cont::CellLocatorUniformGrid locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<vtkm::cont::CellLocatorUniformGrid>(locator);
      }
      else if (coordinates.GetData().IsType<RectilinearType>() &&
               cellset.IsSameType(StructuredType()))
      {
        vtkm::cont::CellLocatorRectilinearGrid locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<vtkm::cont::CellLocatorRectilinearGrid>(locator);
      }
      else
        throw vtkm::cont::ErrorInternal("Cells are not structured.");

      vtkm::cont::StructuredCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::StructuredCellInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
    {
      vtkm::cont::CellLocatorBoundingIntervalHierarchy locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<vtkm::cont::CellLocatorBoundingIntervalHierarchy>(locator);
      vtkm::cont::SingleCellExplicitInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::SingleCellExplicitInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      vtkm::cont::CellLocatorBoundingIntervalHierarchy locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<vtkm::cont::CellLocatorBoundingIntervalHierarchy>(locator);
      vtkm::cont::CellExplicitInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<vtkm::cont::CellExplicitInterpolationHelper>(interpolationHelper);
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
