//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particle_advection_cell_interpolation_helper
#define vtk_m_worklet_particle_advection_cell_interpolation_helper

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>

#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/CellInterpolate.h>

/*
 * Interface to define the helper classes that can return mesh data
 * on a cell by cell basis.
 */
namespace vtkm
{
namespace exec
{

class CellInterpolationHelper : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC_CONT virtual ~CellInterpolationHelper() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC
  virtual void GetCellInfo(const vtkm::Id& cellId,
                           vtkm::UInt8& cellShape,
                           vtkm::IdComponent& numVerts,
                           vtkm::VecVariable<vtkm::Id, 8>& indices) const = 0;
};

class StructuredCellInterpolationHelper : public vtkm::exec::CellInterpolationHelper
{
public:
  StructuredCellInterpolationHelper() = default;

  VTKM_CONT
  StructuredCellInterpolationHelper(vtkm::Id3 cellDims, vtkm::Id3 pointDims, bool is3D)
    : CellDims(cellDims)
    , PointDims(pointDims)
    , Is3D(is3D)
  {
  }

  VTKM_EXEC
  void GetCellInfo(const vtkm::Id& cellId,
                   vtkm::UInt8& cellShape,
                   vtkm::IdComponent& numVerts,
                   vtkm::VecVariable<vtkm::Id, 8>& indices) const override
  {
    vtkm::Id3 logicalCellId;
    logicalCellId[0] = cellId % this->CellDims[0];
    logicalCellId[1] = (cellId / this->CellDims[0]) % this->CellDims[1];
    if (this->Is3D)
    {
      logicalCellId[2] = cellId / (this->CellDims[0] * this->CellDims[1]);
      indices.Append((logicalCellId[2] * this->PointDims[1] + logicalCellId[1]) *
                       this->PointDims[0] +
                     logicalCellId[0]);
      indices.Append(indices[0] + 1);
      indices.Append(indices[1] + this->PointDims[0]);
      indices.Append(indices[2] - 1);
      indices.Append(indices[0] + this->PointDims[0] * this->PointDims[1]);
      indices.Append(indices[4] + 1);
      indices.Append(indices[5] + this->PointDims[0]);
      indices.Append(indices[6] - 1);
      cellShape = static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_HEXAHEDRON);
      numVerts = 8;
    }
    else
    {
      indices.Append(logicalCellId[1] * this->PointDims[0] + logicalCellId[0]);
      indices.Append(indices[0] + 1);
      indices.Append(indices[1] + this->PointDims[0]);
      indices.Append(indices[2] - 1);
      cellShape = static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_QUAD);
      numVerts = 4;
    }
  }

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  bool Is3D = true;
};

template <typename DeviceAdapter>
class SingleCellTypeInterpolationHelper : public vtkm::exec::CellInterpolationHelper
{
  using ConnType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ConnPortalType = typename ConnType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  SingleCellTypeInterpolationHelper() = default;

  VTKM_CONT
  SingleCellTypeInterpolationHelper(vtkm::UInt8 cellShape,
                                    vtkm::IdComponent pointsPerCell,
                                    const ConnType& connectivity)
    : CellShape(cellShape)
    , PointsPerCell(pointsPerCell)
    , Connectivity(connectivity.PrepareForInput(DeviceAdapter()))
  {
  }

  VTKM_EXEC
  void GetCellInfo(const vtkm::Id& cellId,
                   vtkm::UInt8& cellShape,
                   vtkm::IdComponent& numVerts,
                   vtkm::VecVariable<vtkm::Id, 8>& indices) const override
  {
    cellShape = CellShape;
    numVerts = PointsPerCell;
    vtkm::Id n = static_cast<vtkm::Id>(PointsPerCell);
    vtkm::Id offset = cellId * n;

    for (vtkm::Id i = 0; i < n; i++)
      indices.Append(Connectivity.Get(offset + i));
  }

private:
  vtkm::UInt8 CellShape;
  vtkm::IdComponent PointsPerCell;
  ConnPortalType Connectivity;
};

template <typename DeviceAdapter>
class ExplicitCellInterpolationHelper : public vtkm::exec::CellInterpolationHelper
{
  using ShapeType = vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using OffsetType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ConnType = vtkm::cont::ArrayHandle<vtkm::Id>;

  using ShapePortalType = typename ShapeType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using OffsetPortalType = typename OffsetType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using ConnPortalType = typename ConnType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  ExplicitCellInterpolationHelper() = default;

  VTKM_CONT
  ExplicitCellInterpolationHelper(const ShapeType& shape,
                                  const OffsetType& offset,
                                  const ConnType& connectivity)
    : Shape(shape.PrepareForInput(DeviceAdapter()))
    , Offset(offset.PrepareForInput(DeviceAdapter()))
    , Connectivity(connectivity.PrepareForInput(DeviceAdapter()))
  {
  }

  VTKM_EXEC
  void GetCellInfo(const vtkm::Id& cellId,
                   vtkm::UInt8& cellShape,
                   vtkm::IdComponent& numVerts,
                   vtkm::VecVariable<vtkm::Id, 8>& indices) const override
  {
    cellShape = this->Shape.Get(cellId);
    const vtkm::Id offset = this->Offset.Get(cellId);
    numVerts = static_cast<vtkm::IdComponent>(this->Offset.Get(cellId + 1) - offset);

    for (vtkm::IdComponent i = 0; i < numVerts; i++)
      indices.Append(this->Connectivity.Get(offset + i));
  }

private:
  ShapePortalType Shape;
  OffsetPortalType Offset;
  ConnPortalType Connectivity;
};

} // namespace exec

/*
 * Control side base object.
 */
namespace cont
{

class CellInterpolationHelper : public vtkm::cont::ExecutionObjectBase
{
public:
  using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::CellInterpolationHelper>;

  virtual ~CellInterpolationHelper() = default;

  VTKM_CONT virtual const vtkm::exec::CellInterpolationHelper* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const = 0;
};

class StructuredCellInterpolationHelper : public vtkm::cont::CellInterpolationHelper
{
public:
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  StructuredCellInterpolationHelper() = default;

  VTKM_CONT
  StructuredCellInterpolationHelper(const vtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(Structured2DType()))
    {
      this->Is3D = false;
      vtkm::Id2 cellDims =
        cellSet.Cast<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
      vtkm::Id2 pointDims =
        cellSet.Cast<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
      this->CellDims = vtkm::Id3(cellDims[0], cellDims[1], 0);
      this->PointDims = vtkm::Id3(pointDims[0], pointDims[1], 1);
    }
    else if (cellSet.IsSameType(Structured3DType()))
    {
      this->Is3D = true;
      this->CellDims =
        cellSet.Cast<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
      this->PointDims =
        cellSet.Cast<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
    }
    else
      throw vtkm::cont::ErrorBadType("Cell set is not of type CellSetStructured");
  }

  VTKM_CONT
  const vtkm::exec::CellInterpolationHelper* PrepareForExecution(
    vtkm::cont::DeviceAdapterId deviceId) const override
  {
    auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
    const bool valid = tracker.CanRunOn(deviceId);
    if (!valid)
    {
      throwFailedRuntimeDeviceTransfer("StructuredCellInterpolationHelper", deviceId);
    }

    using ExecutionType = vtkm::exec::StructuredCellInterpolationHelper;
    ExecutionType* execObject = new ExecutionType(this->CellDims, this->PointDims, this->Is3D);
    this->ExecHandle.Reset(execObject);

    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  bool Is3D = true;
  mutable HandleType ExecHandle;
};

class SingleCellTypeInterpolationHelper : public vtkm::cont::CellInterpolationHelper
{
public:
  using SingleExplicitType = vtkm::cont::CellSetSingleType<>;

  SingleCellTypeInterpolationHelper() = default;

  VTKM_CONT
  SingleCellTypeInterpolationHelper(const vtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(SingleExplicitType()))
    {
      SingleExplicitType CellSet = cellSet.Cast<SingleExplicitType>();

      const auto cellShapes =
        CellSet.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      const auto numIndices =
        CellSet.GetNumIndicesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

      CellShape = vtkm::cont::ArrayGetValue(0, cellShapes);
      PointsPerCell = vtkm::cont::ArrayGetValue(0, numIndices);
      Connectivity = CellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                  vtkm::TopologyElementTagPoint());
    }
    else
      throw vtkm::cont::ErrorBadType("Cell set is not of type CellSetSingleType");
  }

  struct SingleCellTypeFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              const vtkm::cont::SingleCellTypeInterpolationHelper& contInterpolator,
                              HandleType& execInterpolator) const
    {
      using ExecutionType = vtkm::exec::SingleCellTypeInterpolationHelper<DeviceAdapter>;
      ExecutionType* execObject = new ExecutionType(
        contInterpolator.CellShape, contInterpolator.PointsPerCell, contInterpolator.Connectivity);
      execInterpolator.Reset(execObject);
      return true;
    }
  };

  VTKM_CONT
  const vtkm::exec::CellInterpolationHelper* PrepareForExecution(
    vtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success =
      vtkm::cont::TryExecuteOnDevice(deviceId, SingleCellTypeFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("SingleCellTypeInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  vtkm::UInt8 CellShape;
  vtkm::IdComponent PointsPerCell;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  mutable HandleType ExecHandle;
};

class ExplicitCellInterpolationHelper : public vtkm::cont::CellInterpolationHelper
{
public:
  ExplicitCellInterpolationHelper() = default;

  VTKM_CONT
  ExplicitCellInterpolationHelper(const vtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      vtkm::cont::CellSetExplicit<> CellSet = cellSet.Cast<vtkm::cont::CellSetExplicit<>>();
      Shape =
        CellSet.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      Offset =
        CellSet.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      Connectivity = CellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                  vtkm::TopologyElementTagPoint());
    }
    else
      throw vtkm::cont::ErrorBadType("Cell set is not of type CellSetExplicit");
  }

  struct ExplicitCellFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              const vtkm::cont::ExplicitCellInterpolationHelper& contInterpolator,
                              HandleType& execInterpolator) const
    {
      using ExecutionType = vtkm::exec::ExplicitCellInterpolationHelper<DeviceAdapter>;
      ExecutionType* execObject = new ExecutionType(
        contInterpolator.Shape, contInterpolator.Offset, contInterpolator.Connectivity);
      execInterpolator.Reset(execObject);
      return true;
    }
  };

  VTKM_CONT
  const vtkm::exec::CellInterpolationHelper* PrepareForExecution(
    vtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success =
      vtkm::cont::TryExecuteOnDevice(deviceId, ExplicitCellFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("ExplicitCellInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::UInt8> Shape;
  vtkm::cont::ArrayHandle<vtkm::Id> Offset;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  mutable HandleType ExecHandle;
};

} //namespace cont
} //namespace vtkm

#endif //vtk_m_worklet_particle_advection_cell_interpolation_helper
