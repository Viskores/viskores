//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MeshConnectivityBase
#define vtk_m_rendering_raytracing_MeshConnectivityBase

#include <sstream>
#include <vtkm/CellShape.h>
#include <vtkm/VirtualObjectBase.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/CellTables.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
//
// Base class for different types of face-to-connecting-cell
// and other mesh information
//
class VTKM_ALWAYS_EXPORT MeshConnectivityBase : public VirtualObjectBase
{
public:
  VTKM_EXEC_CONT
  virtual vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const = 0;

  VTKM_EXEC_CONT
  virtual vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const = 0;

  VTKM_EXEC_CONT
  virtual vtkm::UInt8 GetCellShape(const vtkm::Id& cellId) const = 0;
};

// A simple concrete type to wrap MeshConnectivityBase so we can
// pass an ExeObject to worklets.
class MeshWrapper
{
private:
  MeshConnectivityBase* MeshConn;

public:
  MeshWrapper() {}

  MeshWrapper(MeshConnectivityBase* meshConn)
    : MeshConn(meshConn){};

  VTKM_EXEC_CONT
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
  {
    return MeshConn->GetConnectingCell(cellId, face);
  }

  VTKM_EXEC_CONT
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const
  {
    return MeshConn->GetCellIndices(cellIndices, cellId);
  }

  VTKM_EXEC_CONT
  vtkm::UInt8 GetCellShape(const vtkm::Id& cellId) const { return MeshConn->GetCellShape(cellId); }
};

class VTKM_ALWAYS_EXPORT MeshConnStructured : public MeshConnectivityBase
{
protected:
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id4> Id4Handle;
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;

  VTKM_CONT MeshConnStructured() = default;

public:
  VTKM_CONT
  MeshConnStructured(const vtkm::Id3& cellDims, const vtkm::Id3& pointDims)
    : CellDims(cellDims)
    , PointDims(pointDims)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const override
  {
    //TODO: there is probably a better way to do this.
    vtkm::Id3 logicalCellId;
    logicalCellId[0] = cellId % CellDims[0];
    logicalCellId[1] = (cellId / CellDims[0]) % CellDims[1];
    logicalCellId[2] = cellId / (CellDims[0] * CellDims[1]);
    if (face == 0)
      logicalCellId[1] -= 1;
    if (face == 2)
      logicalCellId[1] += 1;
    if (face == 1)
      logicalCellId[0] += 1;
    if (face == 3)
      logicalCellId[0] -= 1;
    if (face == 4)
      logicalCellId[2] -= 1;
    if (face == 5)
      logicalCellId[2] += 1;
    vtkm::Id nextCell =
      (logicalCellId[2] * CellDims[1] + logicalCellId[1]) * CellDims[0] + logicalCellId[0];
    bool validCell = true;
    if (logicalCellId[0] >= CellDims[0])
      validCell = false;
    if (logicalCellId[1] >= CellDims[1])
      validCell = false;
    if (logicalCellId[2] >= CellDims[2])
      validCell = false;
    vtkm::Id minId = vtkm::Min(logicalCellId[0], vtkm::Min(logicalCellId[1], logicalCellId[2]));
    if (minId < 0)
      validCell = false;
    if (!validCell)
      nextCell = -1;
    return nextCell;
  }

  VTKM_EXEC_CONT
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellIndex) const override
  {
    vtkm::Id3 cellId;
    cellId[0] = cellIndex % CellDims[0];
    cellId[1] = (cellIndex / CellDims[0]) % CellDims[1];
    cellId[2] = cellIndex / (CellDims[0] * CellDims[1]);
    cellIndices[0] = (cellId[2] * PointDims[1] + cellId[1]) * PointDims[0] + cellId[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDims[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDims[0] * PointDims[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDims[0];
    cellIndices[7] = cellIndices[6] - 1;
    return 8;
  }

  VTKM_EXEC
  vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const override
  {
    return vtkm::UInt8(CELL_SHAPE_HEXAHEDRON);
  }
}; // MeshConnStructured

template <typename Device>
class VTKM_ALWAYS_EXPORT MeshConnUnstructured : public MeshConnectivityBase
{
protected:
  using IdHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using UCharHandle = typename vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using IdConstPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using UCharConstPortal = typename UCharHandle::ExecutionTypes<Device>::PortalConst;

  // Constant Portals for the execution Environment
  //FaceConn
  IdConstPortal FaceConnPortal;
  IdConstPortal FaceOffsetsPortal;
  //Cell Set
  IdConstPortal CellConnPortal;
  IdConstPortal CellOffsetsPortal;
  UCharConstPortal ShapesPortal;

  VTKM_CONT MeshConnUnstructured() = default;

public:
  VTKM_CONT
  MeshConnUnstructured(const IdHandle& faceConnectivity,
                       const IdHandle& faceOffsets,
                       const IdHandle& cellConn,
                       const IdHandle& cellOffsets,
                       const UCharHandle& shapes)
    : FaceConnPortal(faceConnectivity.PrepareForInput(Device()))
    , FaceOffsetsPortal(faceOffsets.PrepareForInput(Device()))
    , CellConnPortal(cellConn.PrepareForInput(Device()))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(Device()))
    , ShapesPortal(shapes.PrepareForInput(Device()))
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const override
  {
    BOUNDS_CHECK(FaceOffsetsPortal, cellId);
    vtkm::Id cellStartIndex = FaceOffsetsPortal.Get(cellId);
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  //----------------------------------------------------------------------------
  VTKM_EXEC
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const override
  {
    const vtkm::Int32 shapeId = static_cast<vtkm::Int32>(ShapesPortal.Get(cellId));
    CellTables tables;
    const vtkm::Int32 numIndices = tables.FaceLookUp(tables.CellTypeLookUp(shapeId), 2);
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    const vtkm::Id cellOffset = CellOffsetsPortal.Get(cellId);

    for (vtkm::Int32 i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(CellConnPortal, cellOffset + i);
      cellIndices[i] = CellConnPortal.Get(cellOffset + i);
    }
    return numIndices;
  }

  //----------------------------------------------------------------------------
  VTKM_EXEC
  vtkm::UInt8 GetCellShape(const vtkm::Id& cellId) const override
  {
    BOUNDS_CHECK(ShapesPortal, cellId)
    return ShapesPortal.Get(cellId);
  }

}; // MeshConnUnstructured

template <typename Device>
class MeshConnSingleType : public MeshConnectivityBase
{
protected:
  using IdHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using IdConstPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;

  using CountingHandle = typename vtkm::cont::ArrayHandleCounting<vtkm::Id>;
  using CountingPortal = typename CountingHandle::ExecutionTypes<Device>::PortalConst;
  // Constant Portals for the execution Environment
  IdConstPortal FaceConnPortal;
  IdConstPortal CellConnectivityPortal;
  CountingPortal CellOffsetsPortal;

  vtkm::Int32 ShapeId;
  vtkm::Int32 NumIndices;
  vtkm::Int32 NumFaces;

private:
  VTKM_CONT
  MeshConnSingleType() {}

public:
  VTKM_CONT
  MeshConnSingleType(IdHandle& faceConn,
                     IdHandle& cellConn,
                     CountingHandle& cellOffsets,
                     vtkm::Int32 shapeId,
                     vtkm::Int32 numIndices,
                     vtkm::Int32 numFaces)
    : FaceConnPortal(faceConn.PrepareForInput(Device()))
    , CellConnectivityPortal(cellConn.PrepareForInput(Device()))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(Device()))
    , ShapeId(shapeId)
    , NumIndices(numIndices)
    , NumFaces(numFaces)
  {
  }

  //----------------------------------------------------------------------------
  //                       Execution Environment Methods
  //----------------------------------------------------------------------------
  VTKM_EXEC
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const override
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    vtkm::Id cellStartIndex = cellId * NumFaces;
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  VTKM_EXEC
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const override
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    const vtkm::Id cellOffset = CellOffsetsPortal.Get(cellId);

    for (vtkm::Int32 i = 0; i < NumIndices; ++i)
    {
      BOUNDS_CHECK(CellConnectivityPortal, cellOffset + i);
      cellIndices[i] = CellConnectivityPortal.Get(cellOffset + i);
    }

    return NumIndices;
  }

  //----------------------------------------------------------------------------
  VTKM_EXEC
  vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const override
  {
    return vtkm::UInt8(ShapeId);
  }

}; //MeshConn Single type specialization

class VTKM_ALWAYS_EXPORT MeshConnHandle
  : public vtkm::cont::VirtualObjectHandle<MeshConnectivityBase>
{
private:
  using Superclass = vtkm::cont::VirtualObjectHandle<MeshConnectivityBase>;

public:
  MeshConnHandle() = default;

  template <typename MeshConnType, typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  explicit MeshConnHandle(MeshConnType* meshConn,
                          bool aquireOwnership = true,
                          DeviceAdapterList devices = DeviceAdapterList())
    : Superclass(meshConn, aquireOwnership, devices)
  {
  }
};

template <typename MeshConnType, typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
VTKM_CONT MeshConnHandle make_MeshConnHandle(MeshConnType&& func,
                                             DeviceAdapterList devices = DeviceAdapterList())
{
  using IFType = typename std::remove_reference<MeshConnType>::type;
  return MeshConnHandle(new IFType(std::forward<MeshConnType>(func)), true, devices);
}
}
}
} //namespace vtkm::rendering::raytracing

#ifdef VTKM_CUDA

// Cuda seems to have a bug where it expects the template class VirtualObjectTransfer
// to be instantiated in a consistent order among all the translation units of an
// executable. Failing to do so results in random crashes and incorrect results.
// We workaroud this issue by explicitly instantiating VirtualObjectTransfer for
// all the implicit functions here.

#include <vtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::rendering::raytracing::MeshConnStructured);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::rendering::raytracing::MeshConnUnstructured<vtkm::cont::DeviceAdapterTagCuda>);

#endif

#endif // MeshConnectivityBase
