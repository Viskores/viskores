//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  using InnerNodesHandle = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using LeafNodesHandle = vtkm::cont::ArrayHandle<Id>;
  AABBs AABB;
  InnerNodesHandle FlatBVH;
  LeafNodesHandle Leafs;
  vtkm::Bounds TotalBounds;
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

  VTKM_CONT void Allocate(const vtkm::Id& leafCount);

  VTKM_CONT
  void Construct();

  VTKM_CONT
  void SetData(AABBs& aabbs);

  VTKM_CONT
  AABBs& GetAABBs();

  VTKM_CONT
  bool GetIsConstructed() const;

  vtkm::Id GetNumberOfAABBs() const;
}; // class LinearBVH
}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_worklet_BoundingVolumeHierachy_h
