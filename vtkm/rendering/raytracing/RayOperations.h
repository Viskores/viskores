//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Ray_Operations_h
#define vtk_m_rendering_raytracing_Ray_Operations_h

#include <vtkm/Matrix.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/raytracing/ChannelBufferOperations.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/Worklets.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class RayStatusFilter : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  RayStatusFilter() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);
  VTKM_EXEC
  void operator()(const vtkm::Id& hitIndex, vtkm::UInt8& rayStatus) const
  {
    if (hitIndex == -1)
      rayStatus = RAY_EXITED_DOMAIN;
    else if (rayStatus != RAY_EXITED_DOMAIN && rayStatus != RAY_TERMINATED)
      rayStatus = RAY_ACTIVE;
    //else printf("Bad status state %d \n",(int)rayStatus);
  }
}; //class RayStatusFileter

class RayMapCanvas : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Matrix<vtkm::Float32, 4, 4> InverseProjView;
  vtkm::Id Width;
  vtkm::Float32 DoubleInvHeight;
  vtkm::Float32 DoubleInvWidth;
  vtkm::Vec3f_32 Origin;

public:
  VTKM_CONT
  RayMapCanvas(const vtkm::Matrix<vtkm::Float32, 4, 4>& inverseProjView,
               const vtkm::Id width,
               const vtkm::Id height,
               const vtkm::Vec3f_32& origin)
    : InverseProjView(inverseProjView)
    , Width(width)
    , Origin(origin)
  {
    VTKM_ASSERT(width > 0);
    VTKM_ASSERT(height > 0);
    DoubleInvHeight = 2.f / static_cast<vtkm::Float32>(height);
    DoubleInvWidth = 2.f / static_cast<vtkm::Float32>(width);
  }

  using ControlSignature = void(FieldIn, FieldInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename Precision, typename DepthPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pixelId,
                            Precision& maxDistance,
                            const DepthPortalType& depths) const
  {
    vtkm::Vec4f_32 position;
    position[0] = static_cast<vtkm::Float32>(pixelId % Width);
    position[1] = static_cast<vtkm::Float32>(pixelId / Width);
    position[2] = static_cast<vtkm::Float32>(depths.Get(pixelId));
    position[3] = 1;
    // transform into normalized device coordinates (-1,1)
    position[0] = position[0] * DoubleInvWidth - 1.f;
    position[1] = position[1] * DoubleInvHeight - 1.f;
    position[2] = 2.f * position[2] - 1.f;
    // offset so we don't go all the way to the same point
    position[2] -= 0.00001f;
    position = vtkm::MatrixMultiply(InverseProjView, position);
    vtkm::Vec3f_32 p;
    p[0] = position[0] / position[3];
    p[1] = position[1] / position[3];
    p[2] = position[2] / position[3];
    p = p - Origin;

    maxDistance = vtkm::Magnitude(p);
  }
}; //class RayMapMinDistances

} // namespace detail
class RayOperations
{
public:
  template <typename T>
  static void ResetStatus(Ray<T>& rays, vtkm::UInt8 status)
  {
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> statusHandle(status, rays.NumRays);
    vtkm::cont::Algorithm::Copy(statusHandle, rays.Status);
  }

  //
  // Some worklets like triangle intersection do not set the
  // ray status, so this operation sets the status based on
  // the ray hit index
  //
  template <typename Device, typename T>
  static void UpdateRayStatus(Ray<T>& rays, Device)
  {
    vtkm::worklet::DispatcherMapField<detail::RayStatusFilter> dispatcher{ (
      detail::RayStatusFilter{}) };
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(rays.HitIdx, rays.Status);
  }

  template <typename T>
  static void UpdateRayStatus(Ray<T>& rays)
  {
    vtkm::worklet::DispatcherMapField<detail::RayStatusFilter> dispatcher{ (
      detail::RayStatusFilter{}) };
    dispatcher.Invoke(rays.HitIdx, rays.Status);
  }

  static void MapCanvasToRays(Ray<vtkm::Float32>& rays,
                              const vtkm::rendering::Camera& camera,
                              const vtkm::rendering::CanvasRayTracer& canvas);

  template <typename T>
  static vtkm::Id RaysInMesh(Ray<T>& rays)
  {
    vtkm::Vec<UInt8, 2> maskValues;
    maskValues[0] = RAY_ACTIVE;
    maskValues[1] = RAY_LOST;

    vtkm::cont::ArrayHandle<vtkm::UInt8> masks;

    vtkm::worklet::DispatcherMapField<ManyMask<vtkm::UInt8, 2>> dispatcher{ (
      ManyMask<vtkm::UInt8, 2>{ maskValues }) };
    dispatcher.Invoke(rays.Status, masks);
    vtkm::cont::ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::UInt8>> castedMasks(masks);
    const vtkm::Id initVal = 0;
    vtkm::Id count = vtkm::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static vtkm::Id GetStatusCount(Ray<T>& rays, vtkm::Id status)
  {
    vtkm::UInt8 statusUInt8;
    if (status < 0 || status > 255)
    {
      throw vtkm::cont::ErrorBadValue("Rays GetStatusCound: invalid status");
    }

    statusUInt8 = static_cast<vtkm::UInt8>(status);
    vtkm::cont::ArrayHandle<vtkm::UInt8> masks;

    vtkm::worklet::DispatcherMapField<Mask<vtkm::UInt8>> dispatcher{ (
      Mask<vtkm::UInt8>{ statusUInt8 }) };
    dispatcher.Invoke(rays.Status, masks);
    vtkm::cont::ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::UInt8>> castedMasks(masks);
    const vtkm::Id initVal = 0;
    vtkm::Id count = vtkm::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static vtkm::Id RaysProcessed(Ray<T>& rays)
  {
    vtkm::Vec<UInt8, 3> maskValues;
    maskValues[0] = RAY_TERMINATED;
    maskValues[1] = RAY_EXITED_DOMAIN;
    maskValues[2] = RAY_ABANDONED;

    vtkm::cont::ArrayHandle<vtkm::UInt8> masks;

    vtkm::worklet::DispatcherMapField<ManyMask<vtkm::UInt8, 3>> dispatcher{ (
      ManyMask<vtkm::UInt8, 3>{ maskValues }) };
    dispatcher.Invoke(rays.Status, masks);
    vtkm::cont::ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::UInt8>> castedMasks(masks);
    const vtkm::Id initVal = 0;
    vtkm::Id count = vtkm::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static vtkm::cont::ArrayHandle<vtkm::UInt8> CompactActiveRays(Ray<T>& rays)
  {
    vtkm::Vec<UInt8, 1> maskValues;
    maskValues[0] = RAY_ACTIVE;
    vtkm::UInt8 statusUInt8 = static_cast<vtkm::UInt8>(RAY_ACTIVE);
    vtkm::cont::ArrayHandle<vtkm::UInt8> masks;

    vtkm::worklet::DispatcherMapField<Mask<vtkm::UInt8>> dispatcher{ (
      Mask<vtkm::UInt8>{ statusUInt8 }) };
    dispatcher.Invoke(rays.Status, masks);

    vtkm::cont::ArrayHandle<T> emptyHandle;

    rays.Normal =
      vtkm::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);
    rays.Origin =
      vtkm::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);
    rays.Dir = vtkm::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);

    const vtkm::Int32 numFloatArrays = 18;
    vtkm::cont::ArrayHandle<T>* floatArrayPointers[numFloatArrays];
    floatArrayPointers[0] = &rays.OriginX;
    floatArrayPointers[1] = &rays.OriginY;
    floatArrayPointers[2] = &rays.OriginZ;
    floatArrayPointers[3] = &rays.DirX;
    floatArrayPointers[4] = &rays.DirY;
    floatArrayPointers[5] = &rays.DirZ;
    floatArrayPointers[6] = &rays.Distance;
    floatArrayPointers[7] = &rays.MinDistance;
    floatArrayPointers[8] = &rays.MaxDistance;

    floatArrayPointers[9] = &rays.Scalar;
    floatArrayPointers[10] = &rays.IntersectionX;
    floatArrayPointers[11] = &rays.IntersectionY;
    floatArrayPointers[12] = &rays.IntersectionZ;
    floatArrayPointers[13] = &rays.U;
    floatArrayPointers[14] = &rays.V;
    floatArrayPointers[15] = &rays.NormalX;
    floatArrayPointers[16] = &rays.NormalY;
    floatArrayPointers[17] = &rays.NormalZ;

    const int breakPoint = rays.IntersectionDataEnabled ? -1 : 9;
    for (int i = 0; i < numFloatArrays; ++i)
    {
      if (i == breakPoint)
      {
        break;
      }
      vtkm::cont::ArrayHandle<T> compacted;
      vtkm::cont::Algorithm::CopyIf(*floatArrayPointers[i], masks, compacted);
      *floatArrayPointers[i] = compacted;
    }

    //
    // restore the composite vectors
    //
    rays.Normal =
      vtkm::cont::make_ArrayHandleCompositeVector(rays.NormalX, rays.NormalY, rays.NormalZ);
    rays.Origin =
      vtkm::cont::make_ArrayHandleCompositeVector(rays.OriginX, rays.OriginY, rays.OriginZ);
    rays.Dir = vtkm::cont::make_ArrayHandleCompositeVector(rays.DirX, rays.DirY, rays.DirZ);

    vtkm::cont::ArrayHandle<vtkm::Id> compactedHits;
    vtkm::cont::Algorithm::CopyIf(rays.HitIdx, masks, compactedHits);
    rays.HitIdx = compactedHits;

    vtkm::cont::ArrayHandle<vtkm::Id> compactedPixels;
    vtkm::cont::Algorithm::CopyIf(rays.PixelIdx, masks, compactedPixels);
    rays.PixelIdx = compactedPixels;

    vtkm::cont::ArrayHandle<vtkm::UInt8> compactedStatus;
    vtkm::cont::Algorithm::CopyIf(rays.Status, masks, compactedStatus);
    rays.Status = compactedStatus;

    rays.NumRays = rays.Status.GetPortalConstControl().GetNumberOfValues();

    const size_t bufferCount = static_cast<size_t>(rays.Buffers.size());
    for (size_t i = 0; i < bufferCount; ++i)
    {
      ChannelBufferOperations::Compact(rays.Buffers[i], masks, rays.NumRays);
    }
    return masks;
  }

  template <typename Device, typename T>
  static void Resize(Ray<T>& rays, const vtkm::Int32 newSize, Device)
  {
    if (newSize == rays.NumRays)
      return; //nothing to do

    rays.NumRays = newSize;

    if (rays.IntersectionDataEnabled)
    {
      rays.IntersectionX.PrepareForOutput(rays.NumRays, Device());
      rays.IntersectionY.PrepareForOutput(rays.NumRays, Device());
      rays.IntersectionZ.PrepareForOutput(rays.NumRays, Device());
      rays.U.PrepareForOutput(rays.NumRays, Device());
      rays.V.PrepareForOutput(rays.NumRays, Device());
      rays.Scalar.PrepareForOutput(rays.NumRays, Device());

      rays.NormalX.PrepareForOutput(rays.NumRays, Device());
      rays.NormalY.PrepareForOutput(rays.NumRays, Device());
      rays.NormalZ.PrepareForOutput(rays.NumRays, Device());
    }

    rays.OriginX.PrepareForOutput(rays.NumRays, Device());
    rays.OriginY.PrepareForOutput(rays.NumRays, Device());
    rays.OriginZ.PrepareForOutput(rays.NumRays, Device());

    rays.DirX.PrepareForOutput(rays.NumRays, Device());
    rays.DirY.PrepareForOutput(rays.NumRays, Device());
    rays.DirZ.PrepareForOutput(rays.NumRays, Device());

    rays.Distance.PrepareForOutput(rays.NumRays, Device());
    rays.MinDistance.PrepareForOutput(rays.NumRays, Device());
    rays.MaxDistance.PrepareForOutput(rays.NumRays, Device());
    rays.Status.PrepareForOutput(rays.NumRays, Device());
    rays.HitIdx.PrepareForOutput(rays.NumRays, Device());
    rays.PixelIdx.PrepareForOutput(rays.NumRays, Device());

    const size_t bufferCount = static_cast<size_t>(rays.Buffers.size());
    for (size_t i = 0; i < bufferCount; ++i)
    {
      rays.Buffers[i].Resize(rays.NumRays, Device());
    }
  }

  template <typename T>
  static void CopyDistancesToMin(Ray<T> rays, const T offset = 0.f)
  {
    vtkm::worklet::DispatcherMapField<CopyAndOffsetMask<T>> dispatcher{ (
      CopyAndOffsetMask<T>{ offset, RAY_EXITED_MESH }) };
    dispatcher.Invoke(rays.Distance, rays.MinDistance, rays.Status);
  }
};
}
}
} // namespace vktm::rendering::raytracing
#endif
