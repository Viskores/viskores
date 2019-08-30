//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <sstream>
#include <vtkm/CellShape.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBase.h>
#include <vtkm/rendering/raytracing/MeshConnectivityContainers.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

MeshConnContainer::MeshConnContainer(){};
MeshConnContainer::~MeshConnContainer(){};

template <typename T>
VTKM_CONT void MeshConnContainer::FindEntryImpl(Ray<T>& rays)
{
  bool getCellIndex = true;

  Intersector.SetUseWaterTight(true);

  Intersector.IntersectRays(rays, getCellIndex);
}

MeshWrapper MeshConnContainer::PrepareForExecution(const vtkm::cont::DeviceAdapterId deviceId)
{
  return MeshWrapper(const_cast<MeshConnectivityBase*>(this->Construct(deviceId)));
}

void MeshConnContainer::FindEntry(Ray<vtkm::Float32>& rays)
{
  this->FindEntryImpl(rays);
}

void MeshConnContainer::FindEntry(Ray<vtkm::Float64>& rays)
{
  this->FindEntryImpl(rays);
}

VTKM_CONT
UnstructuredContainer::UnstructuredContainer(const vtkm::cont::CellSetExplicit<>& cellset,
                                             const vtkm::cont::CoordinateSystem& coords,
                                             IdHandle& faceConn,
                                             IdHandle& faceOffsets,
                                             Id4Handle& triangles)
  : FaceConnectivity(faceConn)
  , FaceOffsets(faceOffsets)
  , Cellset(cellset)
  , Coords(coords)
{
  this->Triangles = triangles;
  //
  // Grab the cell arrays
  //
  CellConn =
    Cellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  CellOffsets =
    Cellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  Shapes = Cellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

  Intersector.SetData(Coords, Triangles);
}

UnstructuredContainer::~UnstructuredContainer(){};

const MeshConnectivityBase* UnstructuredContainer::Construct(
  const vtkm::cont::DeviceAdapterId deviceId)
{
  switch (deviceId.GetValue())
  {
#ifdef VTKM_ENABLE_OPENMP
    case VTKM_DEVICE_ADAPTER_OPENMP:
      using OMP = vtkm::cont::DeviceAdapterTagOpenMP;
      {
        MeshConnUnstructured<OMP> conn(this->FaceConnectivity,
                                       this->FaceOffsets,
                                       this->CellConn,
                                       this->CellOffsets,
                                       this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(OMP());
#endif
#ifdef VTKM_ENABLE_TBB
    case VTKM_DEVICE_ADAPTER_TBB:
      using TBB = vtkm::cont::DeviceAdapterTagTBB;
      {
        MeshConnUnstructured<TBB> conn(this->FaceConnectivity,
                                       this->FaceOffsets,
                                       this->CellConn,
                                       this->CellOffsets,
                                       this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(TBB());
#endif
#ifdef VTKM_ENABLE_CUDA
    case VTKM_DEVICE_ADAPTER_CUDA:
      using CUDA = vtkm::cont::DeviceAdapterTagCuda;
      {
        MeshConnUnstructured<CUDA> conn(this->FaceConnectivity,
                                        this->FaceOffsets,
                                        this->CellConn,
                                        this->CellOffsets,
                                        this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(CUDA());
#endif
    case VTKM_DEVICE_ADAPTER_SERIAL:
      VTKM_FALLTHROUGH;
    default:
      using SERIAL = vtkm::cont::DeviceAdapterTagSerial;
      {
        MeshConnUnstructured<SERIAL> conn(this->FaceConnectivity,
                                          this->FaceOffsets,
                                          this->CellConn,
                                          this->CellOffsets,
                                          this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(SERIAL());
  }
}

VTKM_CONT
UnstructuredSingleContainer::UnstructuredSingleContainer()
{
}

VTKM_CONT
UnstructuredSingleContainer::UnstructuredSingleContainer(
  const vtkm::cont::CellSetSingleType<>& cellset,
  const vtkm::cont::CoordinateSystem& coords,
  IdHandle& faceConn,
  Id4Handle& triangles)
  : FaceConnectivity(faceConn)
  , Coords(coords)
  , Cellset(cellset)
{

  this->Triangles = triangles;

  this->Intersector.SetUseWaterTight(true);

  CellConnectivity =
    Cellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapes =
    Cellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

  ShapeId = shapes.GetPortalConstControl().Get(0);
  CellTables tables;
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

  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("mesh_conn_construction");

  Intersector.SetData(Coords, Triangles);
}

const MeshConnectivityBase* UnstructuredSingleContainer::Construct(
  const vtkm::cont::DeviceAdapterId deviceId)
{
  switch (deviceId.GetValue())
  {
#ifdef VTKM_ENABLE_OPENMP
    case VTKM_DEVICE_ADAPTER_OPENMP:
      using OMP = vtkm::cont::DeviceAdapterTagOpenMP;
      {
        MeshConnSingleType<OMP> conn(this->FaceConnectivity,
                                     this->CellConnectivity,
                                     this->CellOffsets,
                                     this->ShapeId,
                                     this->NumIndices,
                                     this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(OMP());
#endif
#ifdef VTKM_ENABLE_TBB
    case VTKM_DEVICE_ADAPTER_TBB:
      using TBB = vtkm::cont::DeviceAdapterTagTBB;
      {
        MeshConnSingleType<TBB> conn(this->FaceConnectivity,
                                     this->CellConnectivity,
                                     this->CellOffsets,
                                     this->ShapeId,
                                     this->NumIndices,
                                     this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(TBB());
#endif
#ifdef VTKM_ENABLE_CUDA
    case VTKM_DEVICE_ADAPTER_CUDA:
      using CUDA = vtkm::cont::DeviceAdapterTagCuda;
      {
        MeshConnSingleType<CUDA> conn(this->FaceConnectivity,
                                      this->CellConnectivity,
                                      this->CellOffsets,
                                      this->ShapeId,
                                      this->NumIndices,
                                      this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(CUDA());
#endif
    case VTKM_DEVICE_ADAPTER_SERIAL:
      VTKM_FALLTHROUGH;
    default:
      using SERIAL = vtkm::cont::DeviceAdapterTagSerial;
      {
        MeshConnSingleType<SERIAL> conn(this->FaceConnectivity,
                                        this->CellConnectivity,
                                        this->CellOffsets,
                                        this->ShapeId,
                                        this->NumIndices,
                                        this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(SERIAL());
  }
}

StructuredContainer::StructuredContainer(const vtkm::cont::CellSetStructured<3>& cellset,
                                         const vtkm::cont::CoordinateSystem& coords,
                                         Id4Handle& triangles)
  : Coords(coords)
  , Cellset(cellset)
{

  Triangles = triangles;
  Intersector.SetUseWaterTight(true);

  PointDims = Cellset.GetPointDimensions();
  CellDims = Cellset.GetCellDimensions();

  this->Intersector.SetData(Coords, Triangles);
}

const MeshConnectivityBase* StructuredContainer::Construct(
  const vtkm::cont::DeviceAdapterId deviceId)
{

  MeshConnStructured conn(CellDims, PointDims);
  Handle = make_MeshConnHandle(conn);

  switch (deviceId.GetValue())
  {
#ifdef VTKM_ENABLE_OPENMP
    case VTKM_DEVICE_ADAPTER_OPENMP:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagOpenMP());
#endif
#ifdef VTKM_ENABLE_TBB
    case VTKM_DEVICE_ADAPTER_TBB:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagTBB());
#endif
#ifdef VTKM_ENABLE_CUDA
    case VTKM_DEVICE_ADAPTER_CUDA:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagCuda());
#endif
    case VTKM_DEVICE_ADAPTER_SERIAL:
      VTKM_FALLTHROUGH;
    default:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial());
  }
}
}
}
} //namespace vtkm::rendering::raytracing
