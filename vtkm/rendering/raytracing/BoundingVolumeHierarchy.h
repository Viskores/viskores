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
#ifndef vtk_m_worklet_BoundingVolumeHierachy_h
#define vtk_m_worklet_BoundingVolumeHierachy_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

//
// This is the data structure that is passed to the ray tracer.
//
class VTKM_RENDERING_EXPORT LinearBVH
{
public:
  using InnerNodesHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
  using LeafNodesHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>;
  InnerNodesHandle FlatBVH;
  LeafNodesHandle LeafNodes;
  struct ConstructFunctor;
  vtkm::Id LeafCount;
  vtkm::Bounds CoordBounds;

protected:
  vtkm::cont::ArrayHandleVirtualCoordinates CoordsHandle;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> Triangles;
  bool IsConstructed;
  bool CanConstruct;

public:
  LinearBVH();

  VTKM_CONT
  LinearBVH(vtkm::cont::ArrayHandleVirtualCoordinates coordsHandle,
            vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles,
            vtkm::Bounds coordBounds);

  VTKM_CONT
  LinearBVH(const LinearBVH& other);

  template <typename DeviceAdapter>
  VTKM_CONT void Allocate(const vtkm::Id& leafCount, DeviceAdapter deviceAdapter);

  VTKM_CONT
  void Construct();

  VTKM_CONT
  void SetData(vtkm::cont::ArrayHandleVirtualCoordinates coordsHandle,
               vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles,
               vtkm::Bounds coordBounds);

  template <typename Device>
  VTKM_CONT void ConstructOnDevice(Device device);

  VTKM_CONT
  bool GetIsConstructed() const;

  VTKM_CONT
  vtkm::cont::ArrayHandleVirtualCoordinates GetCoordsHandle() const;

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> GetTriangles() const;

  vtkm::Id GetNumberOfTriangles() const;
}; // class LinearBVH
}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_worklet_BoundingVolumeHierachy_h
