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
  VTKM_EXEC_CONT virtual ~CellInterpolationHelper() noexcept
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
  StructuredCellInterpolationHelper(vtkm::Id3 cellDims, vtkm::Id3 pointDims)
    : CellDims(cellDims)
    , PointDims(pointDims)
  {
  }

  VTKM_EXEC
  void GetCellInfo(const vtkm::Id& cellId,
                   vtkm::UInt8& cellShape,
                   vtkm::IdComponent& numVerts,
                   vtkm::VecVariable<vtkm::Id, 8>& indices) const override
  {
    vtkm::Id3 logicalCellId;
    logicalCellId[0] = cellId % CellDims[0];
    logicalCellId[1] = (cellId / CellDims[0]) % CellDims[1];
    logicalCellId[2] = cellId / (CellDims[0] * CellDims[1]);

    indices.Append((logicalCellId[2] * PointDims[1] + logicalCellId[1]) * PointDims[0] +
                   logicalCellId[0]);
    indices.Append(indices[0] + 1);
    indices.Append(indices[1] + PointDims[0]);
    indices.Append(indices[2] - 1);
    indices.Append(indices[0] + PointDims[0] * PointDims[1]);
    indices.Append(indices[4] + 1);
    indices.Append(indices[5] + PointDims[0]);
    indices.Append(indices[6] - 1);

    cellShape = static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_HEXAHEDRON);
    numVerts = 8;
  }

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
};

template <typename DeviceAdapter>
class SingleCellExplicitInterpolationHelper : public vtkm::exec::CellInterpolationHelper
{
  using ConnType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ConnPortalType = typename ConnType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  SingleCellExplicitInterpolationHelper() = default;

  VTKM_CONT
  SingleCellExplicitInterpolationHelper(vtkm::UInt8 cellShape,
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
class CellExplicitInterpolationHelper : public vtkm::exec::CellInterpolationHelper
{
  using ShapeType = vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using NumIdxType = vtkm::cont::ArrayHandle<vtkm::IdComponent>;
  using OffsetType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ConnType = vtkm::cont::ArrayHandle<vtkm::Id>;

  using ShapePortalType = typename ShapeType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using NumIdxPortalType = typename NumIdxType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using OffsetPortalType = typename OffsetType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using ConnPortalType = typename ConnType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  CellExplicitInterpolationHelper() = default;

  VTKM_CONT
  CellExplicitInterpolationHelper(const ShapeType& shape,
                                  const NumIdxType& numIdx,
                                  const OffsetType& offset,
                                  const ConnType& connectivity)
    : Shape(shape.PrepareForInput(DeviceAdapter()))
    , NumIdx(numIdx.PrepareForInput(DeviceAdapter()))
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
    cellShape = Shape.Get(cellId);
    numVerts = NumIdx.Get(cellId);
    vtkm::Id offset = Offset.Get(cellId);

    for (vtkm::IdComponent i = 0; i < numVerts; i++)
      indices.Append(Connectivity.Get(offset + i));
  }

private:
  ShapePortalType Shape;
  NumIdxPortalType NumIdx;
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
  using StructuredType = vtkm::cont::CellSetStructured<3>;

  StructuredCellInterpolationHelper() = default;

  VTKM_CONT
  StructuredCellInterpolationHelper(const vtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(StructuredType()))
    {
      CellDims = cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
      PointDims =
        cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
    }
    else
      throw vtkm::cont::ErrorBadType("Cell set is not 3D structured type");
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
    ExecutionType* execObject = new ExecutionType(this->CellDims, this->PointDims);
    this->ExecHandle.Reset(execObject);

    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  mutable HandleType ExecHandle;
};

class SingleCellExplicitInterpolationHelper : public vtkm::cont::CellInterpolationHelper
{
public:
  using SingleExplicitType = vtkm::cont::CellSetSingleType<>;

  SingleCellExplicitInterpolationHelper() = default;

  VTKM_CONT
  SingleCellExplicitInterpolationHelper(const vtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(SingleExplicitType()))
    {
      SingleExplicitType CellSet = cellSet.Cast<SingleExplicitType>();
      CellShape =
        CellSet.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell())
          .GetPortalConstControl()
          .Get(0);
      PointsPerCell =
        CellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell())
          .GetPortalConstControl()
          .Get(0);
      Connectivity = CellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                  vtkm::TopologyElementTagCell());
    }
    else
      throw vtkm::cont::ErrorBadType("Cell set is not CellSetSingleType");
  }

  struct SingleCellExplicitFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(
      DeviceAdapter,
      const vtkm::cont::SingleCellExplicitInterpolationHelper& contInterpolator,
      HandleType& execInterpolator) const
    {
      using ExecutionType = vtkm::exec::SingleCellExplicitInterpolationHelper<DeviceAdapter>;
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
    const bool success = vtkm::cont::TryExecuteOnDevice(
      deviceId, SingleCellExplicitFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("SingleCellExplicitInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  vtkm::UInt8 CellShape;
  vtkm::IdComponent PointsPerCell;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  mutable HandleType ExecHandle;
};

class CellExplicitInterpolationHelper : public vtkm::cont::CellInterpolationHelper
{
public:
  CellExplicitInterpolationHelper() = default;

  VTKM_CONT
  CellExplicitInterpolationHelper(const vtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      vtkm::cont::CellSetExplicit<> CellSet = cellSet.Cast<vtkm::cont::CellSetExplicit<>>();
      Shape =
        CellSet.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
      NumIdx =
        CellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
      Offset = CellSet.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(),
                                           vtkm::TopologyElementTagCell());
      Connectivity = CellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                  vtkm::TopologyElementTagCell());
    }
    else
      throw vtkm::cont::ErrorBadType("Cell set is not CellSetSingleType");
  }

  struct SingleCellExplicitFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              const vtkm::cont::CellExplicitInterpolationHelper& contInterpolator,
                              HandleType& execInterpolator) const
    {
      using ExecutionType = vtkm::exec::CellExplicitInterpolationHelper<DeviceAdapter>;
      ExecutionType* execObject = new ExecutionType(contInterpolator.Shape,
                                                    contInterpolator.NumIdx,
                                                    contInterpolator.Offset,
                                                    contInterpolator.Connectivity);
      execInterpolator.Reset(execObject);
      return true;
    }
  };

  VTKM_CONT
  const vtkm::exec::CellInterpolationHelper* PrepareForExecution(
    vtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success = vtkm::cont::TryExecuteOnDevice(
      deviceId, SingleCellExplicitFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("SingleCellExplicitInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::UInt8> Shape;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumIdx;
  vtkm::cont::ArrayHandle<vtkm::Id> Offset;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  mutable HandleType ExecHandle;
};

} //namespace cont
} //namespace vtkm

#endif //vtk_m_worklet_particle_advection_cell_interpolation_helper
