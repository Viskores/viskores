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
    vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>>::type
      Intersection;

  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>>::type Normal;

  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>>::type Origin;

  typename //tell the compiler we have a dependent type
    vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>,
                                               vtkm::cont::ArrayHandle<Precision>>::type Dir;

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
    vtkm::IdComponent inComp[3];
    inComp[0] = 0;
    inComp[1] = 1;
    inComp[2] = 2;
    Intersection = vtkm::cont::make_ArrayHandleCompositeVector(
      IntersectionX, inComp[0], IntersectionY, inComp[1], IntersectionZ, inComp[2]);

    Normal = vtkm::cont::make_ArrayHandleCompositeVector(
      NormalX, inComp[0], NormalY, inComp[1], NormalZ, inComp[2]);

    Origin = vtkm::cont::make_ArrayHandleCompositeVector(
      OriginX, inComp[0], OriginY, inComp[1], OriginZ, inComp[2]);

    Dir = vtkm::cont::make_ArrayHandleCompositeVector(
      DirX, inComp[0], DirY, inComp[1], DirZ, inComp[2]);

    ChannelBuffer<Precision> buffer;
    buffer.Resize(NumRays);
    Buffers.push_back(buffer);
    DebugWidth = -1;
    DebugHeight = -1;
  }

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


  VTKM_CONT void Resize(const vtkm::Int32 size)
  {
    this->Resize(size, vtkm::cont::DeviceAdapterTagSerial());
  }

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

    vtkm::IdComponent inComp[3];
    inComp[0] = 0;
    inComp[1] = 1;
    inComp[2] = 2;

    Intersection = vtkm::cont::make_ArrayHandleCompositeVector(
      IntersectionX, inComp[0], IntersectionY, inComp[1], IntersectionZ, inComp[2]);

    Normal = vtkm::cont::make_ArrayHandleCompositeVector(
      NormalX, inComp[0], NormalY, inComp[1], NormalZ, inComp[2]);

    Origin = vtkm::cont::make_ArrayHandleCompositeVector(
      OriginX, inComp[0], OriginY, inComp[1], OriginZ, inComp[2]);

    Dir = vtkm::cont::make_ArrayHandleCompositeVector(
      DirX, inComp[0], DirY, inComp[1], DirZ, inComp[2]);


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
