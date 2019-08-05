//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MeshConnectivityContainer_h
#define vtk_m_rendering_raytracing_MeshConnectivityContainer_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBase.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class MeshConnContainer : vtkm::cont::ExecutionObjectBase
{
public:
  MeshConnContainer();
  virtual ~MeshConnContainer();

  virtual const MeshConnectivityBase* Construct(const vtkm::cont::DeviceAdapterId deviceId) = 0;

  MeshWrapper PrepareForExecution(const vtkm::cont::DeviceAdapterId deviceId);

  template <typename T>
  VTKM_CONT void FindEntryImpl(Ray<T>& rays);

  void FindEntry(Ray<vtkm::Float32>& rays);

  void FindEntry(Ray<vtkm::Float64>& rays);

protected:
  using Id4Handle = typename vtkm::cont::ArrayHandle<vtkm::Id4>;
  // Mesh Boundary
  Id4Handle Triangles;
  TriangleIntersector Intersector;
  MeshConnHandle Handle;
};

class UnstructuredContainer : public MeshConnContainer
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandle;
  typedef vtkm::cont::ArrayHandle<vtkm::Id4> Id4Handle;
  typedef vtkm::cont::ArrayHandle<vtkm::UInt8> UCharHandle;
  // Control Environment Handles
  // FaceConn
  IdHandle FaceConnectivity;
  IdHandle FaceOffsets;
  //Cell Set
  IdHandle CellConn;
  IdHandle CellOffsets;
  UCharHandle Shapes;

  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CellSetExplicit<> Cellset;
  vtkm::cont::CoordinateSystem Coords;

private:
  VTKM_CONT
  UnstructuredContainer(){};

public:
  VTKM_CONT
  UnstructuredContainer(const vtkm::cont::CellSetExplicit<>& cellset,
                        const vtkm::cont::CoordinateSystem& coords,
                        IdHandle& faceConn,
                        IdHandle& faceOffsets,
                        Id4Handle& triangles);

  virtual ~UnstructuredContainer();

  const MeshConnectivityBase* Construct(const vtkm::cont::DeviceAdapterId deviceId);
};

class StructuredContainer : public MeshConnContainer
{
protected:
  typedef vtkm::cont::ArrayHandle<vtkm::Id4> Id4Handle;
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::CellSetStructured<3> Cellset;

private:
  VTKM_CONT
  StructuredContainer() {}

public:
  VTKM_CONT
  StructuredContainer(const vtkm::cont::CellSetStructured<3>& cellset,
                      const vtkm::cont::CoordinateSystem& coords,
                      Id4Handle& triangles);

  const MeshConnectivityBase* Construct(const vtkm::cont::DeviceAdapterId deviceId) override;

}; //structure mesh conn

class UnstructuredSingleContainer : public MeshConnContainer
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandle;
  typedef vtkm::cont::ArrayHandle<vtkm::Id4> Id4Handle;
  typedef vtkm::cont::ArrayHandleCounting<vtkm::Id> CountingHandle;
  typedef vtkm::cont::ArrayHandleConstant<vtkm::UInt8> ShapesHandle;
  typedef vtkm::cont::ArrayHandleConstant<vtkm::IdComponent> NumIndicesHandle;
  // Control Environment Handles
  IdHandle FaceConnectivity;
  CountingHandle CellOffsets;
  IdHandle CellConnectivity;

  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::CellSetSingleType<> Cellset;

  vtkm::Int32 ShapeId;
  vtkm::Int32 NumIndices;
  vtkm::Int32 NumFaces;

private:
  VTKM_CONT
  UnstructuredSingleContainer();

public:
  VTKM_CONT
  UnstructuredSingleContainer(const vtkm::cont::CellSetSingleType<>& cellset,
                              const vtkm::cont::CoordinateSystem& coords,
                              IdHandle& faceConn,
                              Id4Handle& externalFaces);

  const MeshConnectivityBase* Construct(const vtkm::cont::DeviceAdapterId deviceId) override;

}; //UnstructuredSingleContainer
}
}
} //namespace vtkm::rendering::raytracing
#endif
