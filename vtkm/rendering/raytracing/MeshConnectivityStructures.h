//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MeshConnectivityStructures_h
#define vtk_m_rendering_raytracing_MeshConnectivityStructures_h
#include <sstream>
#include <vtkm/CellShape.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

// MeshConnectivityStrucutures:
// Policy classes for different types of meshes. Each implemented class
// Must implement GetConnetingCell( indexOfCurrentCell, face) that returns
// the index of the cell that connects to the "face" of the current cell.
// Each policy should have a copy constructor to facilitate clean passing
// to worklets (i.e., initialize execution portals if needed).

//
// Primary template for MeshConnExec Object
//

template <typename MeshType, typename Device>
class MeshConnExec
{
};


class UnstructuredMeshConn
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using UCharHandle = vtkm::cont::ArrayHandle<vtkm::UInt8>;
  // Control Environment Handles
  // FaceConn
  IdHandle FaceConnectivity;
  IdHandle FaceOffsets;
  //Cell Set
  IdHandle CellConn;
  IdHandle CellOffsets;
  UCharHandle Shapes;
  // Mesh Boundary
  Id4Handle ExternalTriangles;
  LinearBVH Bvh;

  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CellSetExplicit<> Cellset;
  vtkm::cont::CoordinateSystem Coords;

protected:
  bool IsConstructed;

private:
  VTKM_CONT
  UnstructuredMeshConn(){};

public:
  VTKM_CONT
  UnstructuredMeshConn(const vtkm::cont::DynamicCellSet& cellset,
                       const vtkm::cont::CoordinateSystem& coords)
    : IsConstructed(false)
  {
    Coords = coords;

    if (!cellset.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      throw vtkm::cont::ErrorBadValue(
        "Unstructured Mesh Connecitity Error: not an explicit cell set!");
    }

    Cellset = cellset.Cast<vtkm::cont::CellSetExplicit<>>();



    //
    // Grab the cell arrays
    //
    CellConn =
      Cellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    CellOffsets =
      Cellset.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    Shapes =
      Cellset.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
  }

  //----------------------------------------------------------------------------
  template <typename T>
  VTKM_CONT UnstructuredMeshConn(const T& other)
    : FaceConnectivity(other.FaceConnectivity)
    , FaceOffsets(other.FaceOffsets)
    , CellConn(other.CellConn)
    , CellOffsets(other.CellOffsets)
    , Shapes(other.Shapes)
    , ExternalTriangles(other.ExternalTriangles)
    , Bvh(other.Bvh)
    , CoordinateBounds(other.CoordinateBounds)
    , Cellset(other.Cellset)
    , Coords(other.Coords)
    , IsConstructed(other.IsConstructed)
  {
  }
  //----------------------------------------------------------------------------
  template <typename Device>
  VTKM_CONT void Construct(Device)
  {
    Logger* logger = Logger::GetInstance();
    logger->OpenLogEntry("mesh_conn_construction");
    vtkm::cont::Timer<Device> timer;
    if (!IsConstructed)
    {

      CoordinateBounds = Coords.GetBounds();
      MeshConnectivityBuilder<Device> connBuilder;

      //
      // Build the face-to-face connectivity
      //
      connBuilder.BuildConnectivity(Cellset, Coords.GetData(), CoordinateBounds);

      //
      // Initialize all of the array handles
      //
      FaceConnectivity = connBuilder.GetFaceConnectivity();
      FaceOffsets = connBuilder.GetFaceOffsets();
      ExternalTriangles = connBuilder.GetExternalTriangles();

      //
      // Build BVH on external triangles
      //
      Bvh.SetData(Coords.GetData(), ExternalTriangles, Coords.GetBounds());
      Bvh.ConstructOnDevice(Device());
      IsConstructed = true;
    }

    vtkm::Float64 time = timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }
  //----------------------------------------------------------------------------
  template <typename T, typename Device>
  VTKM_CONT void FindEntry(Ray<T>& rays, Device)
  {
    if (!IsConstructed)
    {
      throw vtkm::cont::ErrorBadValue(
        "Unstructured Mesh Connecitity Error: FindEntry called before Construct");
    }
    TriangleIntersector<Device, TriLeafIntersector<WaterTight<T>>> intersector;
    bool getCellIndex = true;
    intersector.runHitOnly(rays, Bvh, Coords.GetData(), getCellIndex);
  }
  //----------------------------------------------------------------------------
  VTKM_CONT
  vtkm::Id GetNumberOfCells() { return this->Shapes.GetPortalConstControl().GetNumberOfValues(); }

  //----------------------------------------------------------------------------
  //                       Control Environment Methods
  //----------------------------------------------------------------------------
  VTKM_CONT
  Id4Handle GetExternalTriangles() { return ExternalTriangles; }

  //----------------------------------------------------------------------------
  VTKM_CONT
  vtkm::cont::ArrayHandleVirtualCoordinates GetCoordinates() { return Coords.GetData(); }

  //----------------------------------------------------------------------------
  template <typename Device>
  VTKM_CONT vtkm::Bounds GetCoordinateBounds(Device)
  {
    CoordinateBounds = Coords.GetBounds();
    return CoordinateBounds;
  }
  //----------------------------------------------------------------------------
  VTKM_CONT
  bool GetIsConstructed() { return IsConstructed; }
}; //Unstructure mesh conn

template <typename Device>
class MeshConnExec<UnstructuredMeshConn, Device>
{
protected:
  using IdHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using Id4Handle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
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

private:
  VTKM_CONT
  MeshConnExec(){};

public:
  VTKM_CONT
  MeshConnExec(UnstructuredMeshConn& conn)
    : FaceConnPortal(conn.FaceConnectivity.PrepareForInput(Device()))
    , FaceOffsetsPortal(conn.FaceOffsets.PrepareForInput(Device()))
    , CellConnPortal(conn.CellConn.PrepareForInput(Device()))
    , CellOffsetsPortal(conn.CellOffsets.PrepareForInput(Device()))
    , ShapesPortal(conn.Shapes.PrepareForInput(Device()))
  {
    if (!conn.GetIsConstructed())
    {
      throw vtkm::cont::ErrorBadValue(
        "Unstructured Mesh Connecitity Error: GetExecObj called before Construct");
    }
  }

  //----------------------------------------------------------------------------
  template <typename T>
  VTKM_CONT MeshConnExec(const T& other)
    : FaceConnPortal(other.FaceConnPortal)
    , FaceOffsetsPortal(other.FaceConnPortal)
    , CellConnPortal(other.CellConnPortal)
    , CellOffsetsPortal(other.CellOffsetsPortal)
    , ShapesPortal(other.ShapesPortal)
  {
  }
  //----------------------------------------------------------------------------
  //                        Execution Environment Methods
  //----------------------------------------------------------------------------
  VTKM_EXEC
  inline vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
  {
    BOUNDS_CHECK(FaceOffsetsPortal, cellId);
    vtkm::Id cellStartIndex = FaceOffsetsPortal.Get(cellId);
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  //----------------------------------------------------------------------------
  VTKM_EXEC
  inline vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const
  {
    CellTables tables;
    const vtkm::Int32 shapeId = static_cast<vtkm::Int32>(ShapesPortal.Get(cellId));
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
  inline vtkm::UInt8 GetCellShape(const vtkm::Id& cellId) const
  {
    BOUNDS_CHECK(ShapesPortal, cellId)
    return ShapesPortal.Get(cellId);
  }

}; //Unstructure mesh conn exec


// Specialized version for unstructured meshes consisting of
// a single type of cell.
class UnstructuredMeshConnSingleType
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using CountingHandle = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
  using ShapesHandle = vtkm::cont::ArrayHandleConstant<vtkm::UInt8>;
  using NumIndicesHandle = vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>;
  // Control Environment Handles
  IdHandle FaceConnectivity;
  CountingHandle CellOffsets;
  IdHandle CellConnectivity;
  // Mesh Boundary
  LinearBVH Bvh;
  Id4Handle ExternalTriangles;

  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::CellSetSingleType<> Cellset;

  vtkm::Int32 ShapeId;
  vtkm::Int32 NumIndices;
  vtkm::Int32 NumFaces;

protected:
  bool IsConstructed;

private:
  VTKM_CONT
  UnstructuredMeshConnSingleType() {}

public:
  VTKM_CONT
  UnstructuredMeshConnSingleType(const vtkm::cont::DynamicCellSet& cellset,
                                 const vtkm::cont::CoordinateSystem& coords)
    : IsConstructed(false)
  {

    Coords = coords;

    if (!cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
    {
      throw vtkm::cont::ErrorBadValue(
        "Unstructured Mesh Connecitity Single type Error: not an single type cell set!");
    }

    Cellset = cellset.Cast<vtkm::cont::CellSetSingleType<>>();

    CellConnectivity =
      Cellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapes =
      Cellset.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    CellTables tables;
    ShapeId = shapes.GetPortalConstControl().Get(0);
    NumIndices = tables.FaceLookUp(tables.CellTypeLookUp(ShapeId), 2);

    if (NumIndices == 0)
    {
      std::stringstream message;
      message << "Unstructured Mesh Connecitity Single type Error: unsupported cell type: ";
      message << ShapeId;
      throw vtkm::cont::ErrorBadValue(message.str());
    }
    vtkm::Id start = 0;
    NumFaces = tables.FaceLookUp(tables.CellTypeLookUp(ShapeId), 1);
    vtkm::Id numCells = CellConnectivity.GetPortalConstControl().GetNumberOfValues();
    CellOffsets = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(start, NumIndices, numCells);

    //
    // Initialize all of the array portals
    //
  }
  template <typename T>
  VTKM_CONT UnstructuredMeshConnSingleType(const T& other)
    : FaceConnectivity(other.FaceConnectivity)
    , CellOffsets(other.CellOffsets)
    , CellConnectivity(other.CellConnectivity)
    , Bvh(other.Bvh)
    , ExternalTriangles(other.ExternalTriangles)
    , CoordinateBounds(other.CoordinateBounds)
    , Coords(other.coords)
    , Cellset(other.Cellset)
    , ShapeId(other.ShapeId)
    , NumIndices(other.NumIndices)
    , NumFaces(other.NumFaces)
    , IsConstructed(other.IsConstructed)
  {
  }
  //----------------------------------------------------------------------------
  template <typename Device>
  VTKM_CONT void Construct(Device)
  {
    Logger* logger = Logger::GetInstance();
    logger->OpenLogEntry("mesh_conn_construction");
    vtkm::cont::Timer<Device> timer;

    if (!IsConstructed)
    {
      CoordinateBounds = Coords.GetBounds();

      MeshConnectivityBuilder<Device> connBuilder;

      //
      // Build the face-to-face connectivity
      //
      connBuilder.BuildConnectivity(Cellset, Coords.GetData(), CoordinateBounds);
      //
      // Initialize all of the array handles
      //
      FaceConnectivity = connBuilder.GetFaceConnectivity();
      ExternalTriangles = connBuilder.GetExternalTriangles();

      //
      // Build BVH on external triangles
      //
      Bvh.SetData(Coords.GetData(), ExternalTriangles, Coords.GetBounds());
      Bvh.ConstructOnDevice(Device());

      IsConstructed = true;
    }

    vtkm::Float64 time = timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }
  //----------------------------------------------------------------------------

  template <typename T, typename Device>
  VTKM_CONT void FindEntry(Ray<T>& rays, Device)
  {
    if (!IsConstructed)
    {
      throw vtkm::cont::ErrorBadValue(
        "Unstructured Mesh Connecitity Single Error: FindEntry called before Construct");
    }
    TriangleIntersector<Device, TriLeafIntersector<WaterTight<T>>> intersector;
    bool getCellIndex = true;
    intersector.runHitOnly(rays, Bvh, Coords.GetData(), getCellIndex);
  }
  //----------------------------------------------------------------------------
  VTKM_CONT
  vtkm::Id GetNumberOfCells() { return this->Cellset.GetNumberOfCells(); }
  //----------------------------------------------------------------------------
  VTKM_CONT
  Id4Handle GetExternalTriangles() { return ExternalTriangles; }
  //----------------------------------------------------------------------------
  VTKM_CONT
  vtkm::cont::ArrayHandleVirtualCoordinates GetCoordinates() { return Coords.GetData(); }
  //----------------------------------------------------------------------------
  template <typename Device>
  VTKM_CONT vtkm::Bounds GetCoordinateBounds(Device)
  {
    CoordinateBounds = Coords.GetBounds();
    return CoordinateBounds;
  }
  //----------------------------------------------------------------------------
  VTKM_CONT
  bool GetIsConstructed() { return IsConstructed; }
}; //UnstructuredMeshConn Single Type

template <typename Device>
class MeshConnExec<UnstructuredMeshConnSingleType, Device>
{
protected:
  using IdHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using Id4Handle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using CountingHandle = typename vtkm::cont::ArrayHandleCounting<vtkm::Id>;
  using ShapesHandle = typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>;

  using IdConstPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using CountingPortal = typename CountingHandle::ExecutionTypes<Device>::PortalConst;
  using NumIndicesHandle = typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>;
  // Constant Portals for the execution Environment
  IdConstPortal FaceConnPortal;
  IdConstPortal CellConnectivityPortal;
  CountingPortal CellOffsetsPortal;

  vtkm::Int32 ShapeId;
  vtkm::Int32 NumIndices;
  vtkm::Int32 NumFaces;

private:
  VTKM_CONT
  MeshConnExec() {}

public:
  VTKM_CONT
  MeshConnExec(UnstructuredMeshConnSingleType& conn)
    : FaceConnPortal(conn.FaceConnectivity.PrepareForInput(Device()))
    , CellConnectivityPortal(conn.CellConnectivity.PrepareForInput(Device()))
    , CellOffsetsPortal(conn.CellOffsets.PrepareForInput(Device()))
    , ShapeId(conn.ShapeId)
    , NumIndices(conn.NumIndices)
    , NumFaces(conn.NumFaces)
  {
    if (!conn.GetIsConstructed())
    {
      throw vtkm::cont::ErrorBadValue(
        "Unstructured Mesh Connecitity Single Error: GetExecObj called before Construct");
    }
  }
  template <typename T>
  VTKM_CONT MeshConnExec(const T& other)
    : FaceConnPortal(other.FaceConnPortal)
    , CellOffsetsPortal(other.CellOffsetsPortal)
    , CellConnectivityPortal(other.CellConnectivityPortal)
    , ShapeId(other.ShapeId)
    , NumIndices(other.NumIndices)
    , NumFaces(other.NumFaces)
  {
  }
  //----------------------------------------------------------------------------
  //                       Execution Environment Methods
  //----------------------------------------------------------------------------
  VTKM_EXEC
  inline vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    vtkm::Id cellStartIndex = cellId * NumFaces;
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  VTKM_EXEC
  inline vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellId) const
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
  inline vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const
  {
    return vtkm::UInt8(ShapeId);
  }

}; //MeshConn Single type specialization

class StructuredMeshConn
{
public:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::CellSetStructured<3> Cellset;
  // Mesh Boundary
  LinearBVH Bvh;
  Id4Handle ExternalTriangles;

protected:
  bool IsConstructed;

private:
  VTKM_CONT
  StructuredMeshConn() {}

public:
  VTKM_CONT
  StructuredMeshConn(const vtkm::cont::DynamicCellSet& cellset,
                     const vtkm::cont::CoordinateSystem& coords)
    : IsConstructed(false)
  {
    Coords = coords;

    if (!cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      throw vtkm::cont::ErrorBadValue(
        "Structured Mesh Connecitity Error: not a Structured<3> cell set!");
    }

    Cellset = cellset.Cast<vtkm::cont::CellSetStructured<3>>();
    PointDims = Cellset.GetPointDimensions();
    CellDims = Cellset.GetCellDimensions();
  }
  template <typename T>
  VTKM_CONT StructuredMeshConn(const T& other)
    : CellDims(other.CellDims)
    , PointDims(other.PointDims)
    , CoordinateBounds(other.CoordinateBounds)
    , Coords(other.coords)
    , Cellset(other.Cellset)
    , Bvh(other.Bvh)
    , ExternalTriangles(other.ExternalTriangles)
    , IsConstructed(other.IsConstructed)
  {
  }
  //----------------------------------------------------------------------------
  template <typename Device>
  VTKM_CONT void Construct(Device)
  {
    Logger* logger = Logger::GetInstance();
    logger->OpenLogEntry("mesh_conn_construction");

    vtkm::cont::Timer<Device> timer;

    if (!IsConstructed)
    {
      CoordinateBounds = Coords.GetBounds();

      MeshConnectivityBuilder<Device> connBuilder;
      ExternalTriangles = connBuilder.ExternalTrianglesStructured(Cellset);

      //
      // Build BVH on external triangles
      //
      Bvh.SetData(Coords.GetData(), ExternalTriangles, Coords.GetBounds());
      Bvh.ConstructOnDevice(Device());
      IsConstructed = true;
    }

    vtkm::Float64 time = timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }
  //----------------------------------------------------------------------------
  template <typename T, typename Device>
  VTKM_CONT void FindEntry(Ray<T>& rays, Device)
  {
    if (!IsConstructed)
    {
      throw vtkm::cont::ErrorBadValue(
        "Structured Mesh Connecitity Single Error: FindEntry called before Construct");
    }
    TriangleIntersector<Device, TriLeafIntersector<WaterTight<T>>> intersector;
    bool getCellIndex = true;
    intersector.runHitOnly(rays, Bvh, Coords.GetData(), getCellIndex);
  }
  //----------------------------------------------------------------------------
  VTKM_CONT
  vtkm::Id GetNumberOfCells() { return this->CellDims[0] * this->CellDims[1] * this->CellDims[2]; }
  //----------------------------------------------------------------------------
  //                       Control Environment Methods
  //----------------------------------------------------------------------------
  VTKM_CONT
  Id4Handle GetExternalTriangles() { return ExternalTriangles; }

  //----------------------------------------------------------------------------
  VTKM_CONT
  vtkm::cont::ArrayHandleVirtualCoordinates GetCoordinates() { return Coords.GetData(); }

  //----------------------------------------------------------------------------
  template <typename Device>
  VTKM_CONT vtkm::Bounds GetCoordinateBounds(Device)
  {
    CoordinateBounds = Coords.GetBounds();
    return CoordinateBounds;
  }
  //----------------------------------------------------------------------------
  VTKM_CONT
  bool GetIsConstructed() { return IsConstructed; }
}; //structure mesh conn

template <typename Device>
class MeshConnExec<StructuredMeshConn, Device>
{
protected:
  using Id4Handle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Bounds CoordinateBounds;

private:
  VTKM_CONT
  MeshConnExec() {}

public:
  VTKM_CONT
  MeshConnExec(const StructuredMeshConn& other)
    : CellDims(other.CellDims)
    , PointDims(other.PointDims)
    , CoordinateBounds(other.CoordinateBounds)
  {
  }
  template <typename T>
  VTKM_CONT MeshConnExec(const T& other)
    : CellDims(other.CellDims)
    , PointDims(other.PointDims)
    , CoordinateBounds(other.CoordinateBounds)
  {
  }
  //----------------------------------------------------------------------------
  //                       Execution Environment Methods
  //----------------------------------------------------------------------------
  VTKM_EXEC_CONT
  inline vtkm::Id GetConnectingCell(const vtkm::Id& cellId, const vtkm::Id& face) const
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

  //----------------------------------------------------------------------------
  VTKM_EXEC_CONT
  inline vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellIndex) const
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

  //----------------------------------------------------------------------------
  VTKM_EXEC_CONT
  inline vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8],
                                    const vtkm::Vec<vtkm::Id, 3>& cellId) const
  {

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

  //----------------------------------------------------------------------------
  VTKM_EXEC
  inline vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const
  {
    return vtkm::UInt8(CELL_SHAPE_HEXAHEDRON);
  }
}; //Unstructure mesh conn
}
}
} //namespace vtkm::rendering::raytracing
#endif
