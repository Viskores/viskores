//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MeshConnectivity
#define vtk_m_rendering_raytracing_MeshConnectivity

#include <sstream>
#include <vtkm/CellShape.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/exec/internal/Variant.h>
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

class VTKM_ALWAYS_EXPORT MeshConnectivityStructured
{
protected:
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id4> Id4Handle;
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;

public:
  VTKM_CONT
  MeshConnectivityStructured(const vtkm::Id3& cellDims, const vtkm::Id3& pointDims)
    : CellDims(cellDims)
    , PointDims(pointDims)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
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
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellIndex) const
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
  vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const
  {
    return vtkm::UInt8(CELL_SHAPE_HEXAHEDRON);
  }
}; // MeshConnStructured

class VTKM_ALWAYS_EXPORT MeshConnectivityUnstructured
{
protected:
  using IdHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using UCharHandle = typename vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using IdConstPortal = typename IdHandle::ReadPortalType;
  using UCharConstPortal = typename UCharHandle::ReadPortalType;

  // Constant Portals for the execution Environment
  //FaceConn
  IdConstPortal FaceConnPortal;
  IdConstPortal FaceOffsetsPortal;
  //Cell Set
  IdConstPortal CellConnPortal;
  IdConstPortal CellOffsetsPortal;
  UCharConstPortal ShapesPortal;

public:
  VTKM_CONT
  MeshConnectivityUnstructured(const IdHandle& faceConnectivity,
                               const IdHandle& faceOffsets,
                               const IdHandle& cellConn,
                               const IdHandle& cellOffsets,
                               const UCharHandle& shapes,
                               vtkm::cont::DeviceAdapterId device,
                               vtkm::cont::Token& token)
    : FaceConnPortal(faceConnectivity.PrepareForInput(device, token))
    , FaceOffsetsPortal(faceOffsets.PrepareForInput(device, token))
    , CellConnPortal(cellConn.PrepareForInput(device, token))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(device, token))
    , ShapesPortal(shapes.PrepareForInput(device, token))
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
  {
    BOUNDS_CHECK(FaceOffsetsPortal, cellId);
    vtkm::Id cellStartIndex = FaceOffsetsPortal.Get(cellId);
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  //----------------------------------------------------------------------------
  VTKM_EXEC
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const
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
  vtkm::UInt8 GetCellShape(const vtkm::Id& cellId) const
  {
    BOUNDS_CHECK(ShapesPortal, cellId);
    return ShapesPortal.Get(cellId);
  }

}; // MeshConnUnstructured

class MeshConnectivitySingleType
{
protected:
  using IdHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using IdConstPortal = typename IdHandle::ReadPortalType;

  using CountingHandle = typename vtkm::cont::ArrayHandleCounting<vtkm::Id>;
  using CountingPortal = typename CountingHandle::ReadPortalType;
  // Constant Portals for the execution Environment
  IdConstPortal FaceConnPortal;
  IdConstPortal CellConnectivityPortal;
  CountingPortal CellOffsetsPortal;

  vtkm::Int32 ShapeId;
  vtkm::Int32 NumIndices;
  vtkm::Int32 NumFaces;

public:
  VTKM_CONT
  MeshConnectivitySingleType(const IdHandle& faceConn,
                             const IdHandle& cellConn,
                             const CountingHandle& cellOffsets,
                             vtkm::Int32 shapeId,
                             vtkm::Int32 numIndices,
                             vtkm::Int32 numFaces,
                             vtkm::cont::DeviceAdapterId device,
                             vtkm::cont::Token& token)
    : FaceConnPortal(faceConn.PrepareForInput(device, token))
    , CellConnectivityPortal(cellConn.PrepareForInput(device, token))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(device, token))
    , ShapeId(shapeId)
    , NumIndices(numIndices)
    , NumFaces(numFaces)
  {
  }

  //----------------------------------------------------------------------------
  //                       Execution Environment Methods
  //----------------------------------------------------------------------------
  VTKM_EXEC
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    vtkm::Id cellStartIndex = cellId * NumFaces;
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  VTKM_EXEC
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const
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
  vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const
  {
    return vtkm::UInt8(ShapeId);
  }

}; //MeshConn Single type specialization

/// \brief General version of mesh connectivity that can be used for all supported mesh types.
class VTKM_ALWAYS_EXPORT MeshConnectivity
{
  using ConnectivityType = vtkm::exec::internal::
    Variant<MeshConnectivityStructured, MeshConnectivityUnstructured, MeshConnectivitySingleType>;
  ConnectivityType Connectivity;

public:
  // Constructor for structured connectivity
  VTKM_CONT MeshConnectivity(const vtkm::Id3& cellDims, const vtkm::Id3& pointDims)
    : Connectivity(MeshConnectivityStructured(cellDims, pointDims))
  {
  }

  // Constructor for unstructured connectivity
  VTKM_CONT MeshConnectivity(const vtkm::cont::ArrayHandle<vtkm::Id>& faceConnectivity,
                             const vtkm::cont::ArrayHandle<vtkm::Id>& faceOffsets,
                             const vtkm::cont::ArrayHandle<vtkm::Id>& cellConn,
                             const vtkm::cont::ArrayHandle<vtkm::Id>& cellOffsets,
                             const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                             vtkm::cont::DeviceAdapterId device,
                             vtkm::cont::Token& token)
    : Connectivity(MeshConnectivityUnstructured(faceConnectivity,
                                                faceOffsets,
                                                cellConn,
                                                cellOffsets,
                                                shapes,
                                                device,
                                                token))
  {
  }

  // Constructor for unstructured connectivity with single cell type
  VTKM_CONT MeshConnectivity(const vtkm::cont::ArrayHandle<vtkm::Id>& faceConn,
                             const vtkm::cont::ArrayHandle<vtkm::Id>& cellConn,
                             const vtkm::cont::ArrayHandleCounting<vtkm::Id>& cellOffsets,
                             vtkm::Int32 shapeId,
                             vtkm::Int32 numIndices,
                             vtkm::Int32 numFaces,
                             vtkm::cont::DeviceAdapterId device,
                             vtkm::cont::Token& token)
    : Connectivity(MeshConnectivitySingleType(faceConn,
                                              cellConn,
                                              cellOffsets,
                                              shapeId,
                                              numIndices,
                                              numFaces,
                                              device,
                                              token))
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
  {
    return this->Connectivity.CastAndCall(
      [=](auto conn) { return conn.GetConnectingCell(cellId, face); });
  }

  VTKM_EXEC_CONT
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const
  {
    return this->Connectivity.CastAndCall(
      [=](auto conn) { return conn.GetCellIndices(cellIndices, cellId); });
  }

  VTKM_EXEC_CONT
  vtkm::UInt8 GetCellShape(const vtkm::Id& cellId) const
  {
    return this->Connectivity.CastAndCall([=](auto conn) { return conn.GetCellShape(cellId); });
  }
};

}
}
} //namespace vtkm::rendering::raytracing


#endif // MeshConnectivity
