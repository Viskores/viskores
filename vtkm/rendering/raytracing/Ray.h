//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Ray_h
#define vtk_m_rendering_raytracing_Ray_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/rendering/raytracing/ChannelBuffer.h>

#include <vector>

#define RAY_ACTIVE 0
#define RAY_COMPLETE 1
#define RAY_TERMINATED 2
#define RAY_EXITED_MESH 3
#define RAY_EXITED_DOMAIN 4
#define RAY_LOST 5
#define RAY_ABANDONED 6
#define RAY_TUG_EPSILON 0.001

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <typename Precision>
class Ray
{
protected:
  bool IntersectionDataEnabled;

public:
  // composite vectors to hold array handles
  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>>
      Intersection;

  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>>
      Normal;

  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>>
      Origin;

  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>,
                                           vtkm::cont::ArrayHandle<Precision>>
      Dir;

  vtkm::cont::ArrayHandle<Precision> IntersectionX; //ray Intersection
  vtkm::cont::ArrayHandle<Precision> IntersectionY;
  vtkm::cont::ArrayHandle<Precision> IntersectionZ;

  vtkm::cont::ArrayHandle<Precision> OriginX; //ray Origin
  vtkm::cont::ArrayHandle<Precision> OriginY;
  vtkm::cont::ArrayHandle<Precision> OriginZ;

  vtkm::cont::ArrayHandle<Precision> DirX; //ray Dir
  vtkm::cont::ArrayHandle<Precision> DirY;
  vtkm::cont::ArrayHandle<Precision> DirZ;

  vtkm::cont::ArrayHandle<Precision> U; //barycentric coordinates
  vtkm::cont::ArrayHandle<Precision> V;
  vtkm::cont::ArrayHandle<Precision> NormalX; //ray Normal
  vtkm::cont::ArrayHandle<Precision> NormalY;
  vtkm::cont::ArrayHandle<Precision> NormalZ;
  vtkm::cont::ArrayHandle<Precision> Scalar; //scalar

  vtkm::cont::ArrayHandle<Precision> Distance; //distance to hit

  vtkm::cont::ArrayHandle<vtkm::Id> HitIdx;
  vtkm::cont::ArrayHandle<vtkm::Id> PixelIdx;

  vtkm::cont::ArrayHandle<Precision> MinDistance; // distance to hit
  vtkm::cont::ArrayHandle<Precision> MaxDistance; // distance to hit
  vtkm::cont::ArrayHandle<vtkm::UInt8> Status;    // 0 = active 1 = miss 2 = lost

  std::vector<ChannelBuffer<Precision>> Buffers;
  vtkm::Id DebugWidth;
  vtkm::Id DebugHeight;
  vtkm::Id NumRays;

  VTKM_CONT
  Ray()
  {
    IntersectionDataEnabled = false;
    NumRays = 0;
    Intersection =
      vtkm::cont::make_ArrayHandleCompositeVector(IntersectionX, IntersectionY, IntersectionZ);
    Normal = vtkm::cont::make_ArrayHandleCompositeVector(NormalX, NormalY, NormalZ);
    Origin = vtkm::cont::make_ArrayHandleCompositeVector(OriginX, OriginY, OriginZ);
    Dir = vtkm::cont::make_ArrayHandleCompositeVector(DirX, DirY, DirZ);

    ChannelBuffer<Precision> buffer;
    buffer.Resize(NumRays);
    Buffers.push_back(buffer);
    DebugWidth = -1;
    DebugHeight = -1;
  }

  void EnableIntersectionData()
  {
    if (IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = true;

    IntersectionX.Allocate(NumRays);
    IntersectionY.Allocate(NumRays);
    IntersectionZ.Allocate(NumRays);

    U.Allocate(NumRays);
    V.Allocate(NumRays);
    Scalar.Allocate(NumRays);

    NormalX.Allocate(NumRays);
    NormalY.Allocate(NumRays);
    NormalZ.Allocate(NumRays);
  }

  void DisableIntersectionData()
  {
    if (!IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = false;
    IntersectionX.ReleaseResources();
    IntersectionY.ReleaseResources();
    IntersectionZ.ReleaseResources();
    U.ReleaseResources();
    V.ReleaseResources();
    Scalar.ReleaseResources();

    NormalX.ReleaseResources();
    NormalY.ReleaseResources();
    NormalZ.ReleaseResources();
  }

  VTKM_CONT
  void AddBuffer(const vtkm::Int32 numChannels, const std::string name)
  {
    ChannelBuffer<Precision> buffer(numChannels, this->NumRays);
    buffer.SetName(name);
    this->Buffers.push_back(buffer);
  }

  VTKM_CONT
  bool HasBuffer(const std::string name)
  {
    for (const auto& buffer : this->Buffers)
    {
      if (buffer.GetName() == name)
        return true;
    }
    return false;
  }

  VTKM_CONT
  ChannelBuffer<Precision>& GetBuffer(const std::string name)
  {
    for (auto&& buffer : this->Buffers)
    {
      if (buffer.GetName() == name)
        return buffer;
    }

    throw vtkm::cont::ErrorBadValue("No channel buffer with requested name: " + name);
  }

  void PrintRay(vtkm::Id pixelId)
  {
    for (vtkm::Id i = 0; i < NumRays; ++i)
    {
      if (PixelIdx.WritePortal().Get(i) == pixelId)
      {
        std::cout << "Ray " << pixelId << "\n";
        std::cout << "Origin "
                  << "[" << OriginX.WritePortal().Get(i) << "," << OriginY.WritePortal().Get(i)
                  << "," << OriginZ.WritePortal().Get(i) << "]\n";
        std::cout << "Dir "
                  << "[" << DirX.WritePortal().Get(i) << "," << DirY.WritePortal().Get(i) << ","
                  << DirZ.WritePortal().Get(i) << "]\n";
      }
    }
  }

  friend class RayOperations;
}; // class ray
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Ray_h
