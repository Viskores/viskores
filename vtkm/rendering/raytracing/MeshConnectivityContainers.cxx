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
VTKM_CONT void MeshConnContainer::FindEntryImpl(Ray<T>& rays,
                                                const vtkm::cont::DeviceAdapterId deviceId)
{
  bool getCellIndex = true;

  Intersector.SetUseWaterTight(true);

  switch (deviceId.GetValue())
  {
#ifdef VTKM_ENABLE_TBB
    case VTKM_DEVICE_ADAPTER_TBB:
      Intersector.IntersectRays(rays, vtkm::cont::DeviceAdapterTagTBB(), getCellIndex);
      break;
#endif
#ifdef VTKM_ENABLE_CUDA
    case VTKM_DEVICE_ADAPTER_CUDA:
      Intersector.IntersectRays(rays, vtkm::cont::DeviceAdapterTagCuda(), getCellIndex);
      break;
#endif
    default:
      Intersector.IntersectRays(rays, vtkm::cont::DeviceAdapterTagSerial(), getCellIndex);
      break;
  }
}

void MeshConnContainer::FindEntry(Ray<vtkm::Float32>& rays,
                                  const vtkm::cont::DeviceAdapterId deviceId)
{
  this->FindEntryImpl(rays, deviceId);
}

void MeshConnContainer::FindEntry(Ray<vtkm::Float64>& rays,
                                  const vtkm::cont::DeviceAdapterId deviceId)
{
  this->FindEntryImpl(rays, deviceId);
}

VTKM_CONT
UnstructuredContainer::UnstructuredContainer(const vtkm::cont::CellSetExplicit<>& cellset,
                                             const vtkm::cont::CoordinateSystem& coords,
                                             IdHandle& faceConn,
                                             IdHandle& faceOffsets,
                                             Id4Handle& externalTriangles)
  : FaceConnectivity(faceConn)
  , FaceOffsets(faceOffsets)
  , Cellset(cellset)
  , Coords(coords)
{
  this->ExternalTriangles = externalTriangles;
  //
  // Grab the cell arrays
  //
  CellConn =
    Cellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
  CellOffsets =
    Cellset.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
  Shapes = Cellset.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

  Intersector.SetData(Coords, ExternalTriangles);
}

UnstructuredContainer::~UnstructuredContainer(){};

const MeshConnectivityBase* UnstructuredContainer::Construct(
  const vtkm::cont::DeviceAdapterId deviceId)
{
  switch (deviceId.GetValue())
  {
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
  Id4Handle& externalTriangles)
  : FaceConnectivity(faceConn)
  , Coords(coords)
  , Cellset(cellset)
{

  this->ExternalTriangles = externalTriangles;

  this->Intersector.SetUseWaterTight(true);

  CellConnectivity =
    Cellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
  vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapes =
    Cellset.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

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
  vtkm::cont::Timer<cont::DeviceAdapterTagSerial> timer;

  Intersector.SetData(Coords, ExternalTriangles);
}

const MeshConnectivityBase* UnstructuredSingleContainer::Construct(
  const vtkm::cont::DeviceAdapterId deviceId)
{
  switch (deviceId.GetValue())
  {
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
                                         Id4Handle& externalTriangles)
  : Coords(coords)
  , Cellset(cellset)
{

  ExternalTriangles = externalTriangles;
  Intersector.SetUseWaterTight(true);

  PointDims = Cellset.GetPointDimensions();
  CellDims = Cellset.GetCellDimensions();

  this->Intersector.SetData(Coords, ExternalTriangles);
}

const MeshConnectivityBase* StructuredContainer::Construct(
  const vtkm::cont::DeviceAdapterId deviceId)
{

  MeshConnStructured conn(CellDims, PointDims);
  Handle = make_MeshConnHandle(conn);

  switch (deviceId.GetValue())
  {
#ifdef VTKM_ENABLE_TBB
    case VTKM_DEVICE_ADAPTER_TBB:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagTBB());
#endif
#ifdef VTKM_ENABLE_CUDA
    case VTKM_DEVICE_ADAPTER_CUDA:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagCuda());
#endif
    default:
      return Handle.PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial());
  }
}
}
}
} //namespace vtkm::rendering::raytracing
