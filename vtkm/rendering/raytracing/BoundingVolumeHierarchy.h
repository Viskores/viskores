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

struct AABBs
{
  vtkm::cont::ArrayHandle<vtkm::Float32> xmins;
  vtkm::cont::ArrayHandle<vtkm::Float32> ymins;
  vtkm::cont::ArrayHandle<vtkm::Float32> zmins;
  vtkm::cont::ArrayHandle<vtkm::Float32> xmaxs;
  vtkm::cont::ArrayHandle<vtkm::Float32> ymaxs;
  vtkm::cont::ArrayHandle<vtkm::Float32> zmaxs;
};

//
// This is the data structure that is passed to the ray tracer.
//
class VTKM_RENDERING_EXPORT LinearBVH
{
public:
  using InnerNodesHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
  using LeafNodesHandle = vtkm::cont::ArrayHandle<Id>;
  AABBs AABB;
  InnerNodesHandle FlatBVH;
  LeafNodesHandle Leafs;
  vtkm::Bounds TotalBounds;
  struct ConstructFunctor;
  vtkm::Id LeafCount;

protected:
  bool IsConstructed;
  bool CanConstruct;

public:
  LinearBVH();

  VTKM_CONT
  LinearBVH(AABBs& aabbs);

  VTKM_CONT
  LinearBVH(const LinearBVH& other);

  template <typename DeviceAdapter>
  VTKM_CONT void Allocate(const vtkm::Id& leafCount, DeviceAdapter deviceAdapter);

  VTKM_CONT
  void Construct();

  VTKM_CONT
  void SetData(AABBs& aabbs);

  VTKM_CONT
  AABBs& GetAABBs();

  template <typename Device>
  VTKM_CONT void ConstructOnDevice(Device device);

  VTKM_CONT
  bool GetIsConstructed() const;

  vtkm::Id GetNumberOfAABBs() const;
}; // class LinearBVH
}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_worklet_BoundingVolumeHierachy_h
