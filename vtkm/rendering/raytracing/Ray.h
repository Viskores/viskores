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
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/rendering/raytracing/ChannelBuffer.h>
#include <vtkm/rendering/raytracing/Worklets.h>

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


  struct EnableIntersectionDataFunctor
  {
    template <typename Device>
    VTKM_CONT bool operator()(Device, Ray<Precision>* self)
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(Device);
      self->EnableIntersectionData(Device());
      return true;
    }
  };

  void EnableIntersectionData() { vtkm::cont::TryExecute(EnableIntersectionDataFunctor(), this); }

  template <typename Device>
  void EnableIntersectionData(Device)
  {
    if (IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = true;
    IntersectionX.PrepareForOutput(NumRays, Device());
    IntersectionY.PrepareForOutput(NumRays, Device());
    IntersectionZ.PrepareForOutput(NumRays, Device());
    U.PrepareForOutput(NumRays, Device());
    V.PrepareForOutput(NumRays, Device());
    Scalar.PrepareForOutput(NumRays, Device());

    NormalX.PrepareForOutput(NumRays, Device());
    NormalY.PrepareForOutput(NumRays, Device());
    NormalZ.PrepareForOutput(NumRays, Device());
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

  template <typename Device>
  VTKM_CONT Ray(const vtkm::Int32 size, Device, bool enableIntersectionData = false)
  {
    NumRays = size;
    IntersectionDataEnabled = enableIntersectionData;

    ChannelBuffer<Precision> buffer;
    this->Buffers.push_back(buffer);

    DebugWidth = -1;
    DebugHeight = -1;

    this->Resize(size, Device());
  }

  struct ResizeFunctor
  {
    template <typename Device>
    VTKM_CONT bool operator()(Device, Ray<Precision>* self, const vtkm::Int32 size)
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(Device);
      self->Resize(size, Device());
      return true;
    }
  };

  VTKM_CONT void Resize(const vtkm::Int32 size) { vtkm::cont::TryExecute(ResizeFunctor(), size); }

  template <typename Device>
  VTKM_CONT void Resize(const vtkm::Int32 size, Device)
  {
    NumRays = size;

    if (IntersectionDataEnabled)
    {
      IntersectionX.PrepareForOutput(NumRays, Device());
      IntersectionY.PrepareForOutput(NumRays, Device());
      IntersectionZ.PrepareForOutput(NumRays, Device());

      U.PrepareForOutput(NumRays, Device());
      V.PrepareForOutput(NumRays, Device());

      Scalar.PrepareForOutput(NumRays, Device());

      NormalX.PrepareForOutput(NumRays, Device());
      NormalY.PrepareForOutput(NumRays, Device());
      NormalZ.PrepareForOutput(NumRays, Device());
    }

    OriginX.PrepareForOutput(NumRays, Device());
    OriginY.PrepareForOutput(NumRays, Device());
    OriginZ.PrepareForOutput(NumRays, Device());

    DirX.PrepareForOutput(NumRays, Device());
    DirY.PrepareForOutput(NumRays, Device());
    DirZ.PrepareForOutput(NumRays, Device());

    Distance.PrepareForOutput(NumRays, Device());

    MinDistance.PrepareForOutput(NumRays, Device());
    MaxDistance.PrepareForOutput(NumRays, Device());
    Status.PrepareForOutput(NumRays, Device());

    HitIdx.PrepareForOutput(NumRays, Device());
    PixelIdx.PrepareForOutput(NumRays, Device());

    Intersection =
      vtkm::cont::make_ArrayHandleCompositeVector(IntersectionX, IntersectionY, IntersectionZ);
    Normal = vtkm::cont::make_ArrayHandleCompositeVector(NormalX, NormalY, NormalZ);
    Origin = vtkm::cont::make_ArrayHandleCompositeVector(OriginX, OriginY, OriginZ);
    Dir = vtkm::cont::make_ArrayHandleCompositeVector(DirX, DirY, DirZ);

    const size_t numBuffers = this->Buffers.size();
    for (size_t i = 0; i < numBuffers; ++i)
    {
      this->Buffers[i].Resize(NumRays, Device());
    }
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
    size_t numBuffers = this->Buffers.size();
    bool found = false;
    for (size_t i = 0; i < numBuffers; ++i)
    {
      if (this->Buffers[i].GetName() == name)
      {
        found = true;
        break;
      }
    }
    return found;
  }

  VTKM_CONT
  ChannelBuffer<Precision>& GetBuffer(const std::string name)
  {
    const size_t numBuffers = this->Buffers.size();
    bool found = false;
    size_t index = 0;
    for (size_t i = 0; i < numBuffers; ++i)
    {
      if (this->Buffers[i].GetName() == name)
      {
        found = true;
        index = i;
      }
    }
    if (found)
    {
      return this->Buffers.at(index);
    }
    else
    {
      throw vtkm::cont::ErrorBadValue("No channel buffer with requested name: " + name);
    }
  }

  void PrintRay(vtkm::Id pixelId)
  {
    for (vtkm::Id i = 0; i < NumRays; ++i)
    {
      if (PixelIdx.GetPortalControl().Get(i) == pixelId)
      {
        std::cout << "Ray " << pixelId << "\n";
        std::cout << "Origin "
                  << "[" << OriginX.GetPortalControl().Get(i) << ","
                  << OriginY.GetPortalControl().Get(i) << "," << OriginZ.GetPortalControl().Get(i)
                  << "]\n";
        std::cout << "Dir "
                  << "[" << DirX.GetPortalControl().Get(i) << "," << DirY.GetPortalControl().Get(i)
                  << "," << DirZ.GetPortalControl().Get(i) << "]\n";
      }
    }
  }

  friend class RayOperations;
}; // class ray
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Ray_h
