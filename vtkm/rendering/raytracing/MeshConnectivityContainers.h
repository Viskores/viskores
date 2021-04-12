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
#include <vtkm/rendering/raytracing/MeshConnectivity.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class MeshConnectivityContainer : vtkm::cont::ExecutionObjectBase
{
public:
  MeshConnectivityContainer();
  virtual ~MeshConnectivityContainer();

  virtual MeshConnectivity PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                               vtkm::cont::Token& token) const = 0;

  void FindEntry(Ray<vtkm::Float32>& rays);

  void FindEntry(Ray<vtkm::Float64>& rays);

protected:
  using Id4Handle = typename vtkm::cont::ArrayHandle<vtkm::Id4>;
  // Mesh Boundary
  Id4Handle Triangles;
  TriangleIntersector Intersector;

private:
  template <typename T>
  VTKM_CONT void FindEntryImpl(Ray<T>& rays);
};

class MeshConnectivityContainerUnstructured : public MeshConnectivityContainer
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

public:
  VTKM_CONT
  MeshConnectivityContainerUnstructured(const vtkm::cont::CellSetExplicit<>& cellset,
                                        const vtkm::cont::CoordinateSystem& coords,
                                        const IdHandle& faceConn,
                                        const IdHandle& faceOffsets,
                                        const Id4Handle& triangles);

  virtual ~MeshConnectivityContainerUnstructured();

  MeshConnectivity PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                       vtkm::cont::Token& token) const override;
};

class MeshConnectivityContainerStructured : public MeshConnectivityContainer
{
protected:
  typedef vtkm::cont::ArrayHandle<vtkm::Id4> Id4Handle;
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Bounds CoordinateBounds;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::CellSetStructured<3> Cellset;

public:
  VTKM_CONT
  MeshConnectivityContainerStructured(const vtkm::cont::CellSetStructured<3>& cellset,
                                      const vtkm::cont::CoordinateSystem& coords,
                                      const Id4Handle& triangles);

  MeshConnectivity PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                       vtkm::cont::Token& token) const override;

}; //structure mesh conn

class MeshConnectivityContainerSingleType : public MeshConnectivityContainer
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

public:
  VTKM_CONT
  MeshConnectivityContainerSingleType(const vtkm::cont::CellSetSingleType<>& cellset,
                                      const vtkm::cont::CoordinateSystem& coords,
                                      const IdHandle& faceConn,
                                      const Id4Handle& externalFaces);

  MeshConnectivity PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                       vtkm::cont::Token& token) const override;

}; //UnstructuredSingleContainer
}
}
} //namespace vtkm::rendering::raytracing
#endif
