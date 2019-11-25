//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Sampler.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <limits>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class PixelData : public vtkm::worklet::WorkletMapField
{
public:
  vtkm::Int32 w;
  vtkm::Int32 h;
  vtkm::Int32 Minx;
  vtkm::Int32 Miny;
  vtkm::Int32 SubsetWidth;
  vtkm::Vec3f_32 nlook; // normalized look
  vtkm::Vec3f_32 delta_x;
  vtkm::Vec3f_32 delta_y;
  vtkm::Vec3f_32 Origin;
  vtkm::Bounds BoundingBox;
  VTKM_CONT
  PixelData(vtkm::Int32 width,
            vtkm::Int32 height,
            vtkm::Float32 fovX,
            vtkm::Float32 fovY,
            vtkm::Vec3f_32 look,
            vtkm::Vec3f_32 up,
            vtkm::Float32 _zoom,
            vtkm::Int32 subsetWidth,
            vtkm::Int32 minx,
            vtkm::Int32 miny,
            vtkm::Vec3f_32 origin,
            vtkm::Bounds boundingBox)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
    , Origin(origin)
    , BoundingBox(boundingBox)
  {
    vtkm::Float32 thx = tanf((fovX * vtkm::Pi_180f()) * .5f);
    vtkm::Float32 thy = tanf((fovY * vtkm::Pi_180f()) * .5f);
    vtkm::Vec3f_32 ru = vtkm::Cross(look, up);
    vtkm::Normalize(ru);

    vtkm::Vec3f_32 rv = vtkm::Cross(ru, look);
    vtkm::Normalize(rv);
    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    vtkm::Normalize(nlook);
  }

  VTKM_EXEC inline vtkm::Float32 rcp(vtkm::Float32 f) const { return 1.0f / f; }

  VTKM_EXEC inline vtkm::Float32 rcp_safe(vtkm::Float32 f) const
  {
    return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
  }

  using ControlSignature = void(FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2);
  VTKM_EXEC
  void operator()(const vtkm::Id idx, vtkm::Int32& hit, vtkm::Float32& distance) const
  {
    vtkm::Vec3f_32 ray_dir;
    int i = vtkm::Int32(idx) % SubsetWidth;
    int j = vtkm::Int32(idx) / SubsetWidth;
    i += Minx;
    j += Miny;
    // Write out the global pixelId
    ray_dir = nlook + delta_x * ((2.f * vtkm::Float32(i) - vtkm::Float32(w)) / 2.0f) +
      delta_y * ((2.f * vtkm::Float32(j) - vtkm::Float32(h)) / 2.0f);

    vtkm::Float32 dot = vtkm::Dot(ray_dir, ray_dir);
    vtkm::Float32 sq_mag = vtkm::Sqrt(dot);

    ray_dir[0] = ray_dir[0] / sq_mag;
    ray_dir[1] = ray_dir[1] / sq_mag;
    ray_dir[2] = ray_dir[2] / sq_mag;

    vtkm::Float32 invDirx = rcp_safe(ray_dir[0]);
    vtkm::Float32 invDiry = rcp_safe(ray_dir[1]);
    vtkm::Float32 invDirz = rcp_safe(ray_dir[2]);

    vtkm::Float32 odirx = Origin[0] * invDirx;
    vtkm::Float32 odiry = Origin[1] * invDiry;
    vtkm::Float32 odirz = Origin[2] * invDirz;

    vtkm::Float32 xmin = vtkm::Float32(BoundingBox.X.Min) * invDirx - odirx;
    vtkm::Float32 ymin = vtkm::Float32(BoundingBox.Y.Min) * invDiry - odiry;
    vtkm::Float32 zmin = vtkm::Float32(BoundingBox.Z.Min) * invDirz - odirz;
    vtkm::Float32 xmax = vtkm::Float32(BoundingBox.X.Max) * invDirx - odirx;
    vtkm::Float32 ymax = vtkm::Float32(BoundingBox.Y.Max) * invDiry - odiry;
    vtkm::Float32 zmax = vtkm::Float32(BoundingBox.Z.Max) * invDirz - odirz;
    vtkm::Float32 mind = vtkm::Max(
      vtkm::Max(vtkm::Max(vtkm::Min(ymin, ymax), vtkm::Min(xmin, xmax)), vtkm::Min(zmin, zmax)),
      0.f);
    vtkm::Float32 maxd =
      vtkm::Min(vtkm::Min(vtkm::Max(ymin, ymax), vtkm::Max(xmin, xmax)), vtkm::Max(zmin, zmax));
    if (maxd < mind)
    {
      hit = 0;
      distance = 0;
    }
    else
    {
      distance = maxd - mind;
      hit = 1;
    }
  }

}; // class pixelData

class PerspectiveRayGenJitter : public vtkm::worklet::WorkletMapField
{
public:
  vtkm::Int32 w;
  vtkm::Int32 h;
  vtkm::Vec3f_32 nlook; // normalized look
  vtkm::Vec3f_32 delta_x;
  vtkm::Vec3f_32 delta_y;
  vtkm::Int32 CurrentSample;
  VTKM_CONT_EXPORT
  PerspectiveRayGenJitter(vtkm::Int32 width,
                          vtkm::Int32 height,
                          vtkm::Float32 fovX,
                          vtkm::Float32 fovY,
                          vtkm::Vec3f_32 look,
                          vtkm::Vec3f_32 up,
                          vtkm::Float32 _zoom,
                          vtkm::Int32 currentSample)
    : w(width)
    , h(height)
  {
    vtkm::Float32 thx = tanf((fovX * 3.1415926f / 180.f) * .5f);
    vtkm::Float32 thy = tanf((fovY * 3.1415926f / 180.f) * .5f);
    vtkm::Vec3f_32 ru = vtkm::Cross(up, look);
    vtkm::Normalize(ru);

    vtkm::Vec3f_32 rv = vtkm::Cross(ru, look);
    vtkm::Normalize(rv);

    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    vtkm::Normalize(nlook);
    CurrentSample = currentSample;
  }

  typedef void ControlSignature(FieldOut, FieldOut, FieldOut, FieldIn);

  typedef void ExecutionSignature(WorkIndex, _1, _2, _3, _4);
  VTKM_EXEC
  void operator()(vtkm::Id idx,
                  vtkm::Float32& rayDirX,
                  vtkm::Float32& rayDirY,
                  vtkm::Float32& rayDirZ,
                  const vtkm::Int32& seed) const
  {
    vtkm::Vec2f_32 xy;
    Halton2D<3>(CurrentSample + seed, xy);
    xy[0] -= .5f;
    xy[1] -= .5f;

    vtkm::Vec3f_32 ray_dir(rayDirX, rayDirY, rayDirZ);
    vtkm::Float32 i = static_cast<vtkm::Float32>(vtkm::Int32(idx) % w);
    vtkm::Float32 j = static_cast<vtkm::Float32>(vtkm::Int32(idx) / w);
    i += xy[0];
    j += xy[1];
    ray_dir = nlook + delta_x * ((2.f * i - vtkm::Float32(w)) / 2.0f) +
      delta_y * ((2.f * j - vtkm::Float32(h)) / 2.0f);
    vtkm::Normalize(ray_dir);
    rayDirX = ray_dir[0];
    rayDirY = ray_dir[1];
    rayDirZ = ray_dir[2];
  }

}; // class perspective ray gen jitter

class Camera::Ortho2DRayGen : public vtkm::worklet::WorkletMapField
{
public:
  vtkm::Int32 w;
  vtkm::Int32 h;
  vtkm::Int32 Minx;
  vtkm::Int32 Miny;
  vtkm::Int32 SubsetWidth;
  vtkm::Vec3f_32 nlook; // normalized look
  vtkm::Vec3f_32 PixelDelta;
  vtkm::Vec3f_32 delta_y;
  vtkm::Vec3f_32 StartOffset;

  VTKM_CONT
  Ortho2DRayGen(vtkm::Int32 width,
                vtkm::Int32 height,
                vtkm::Float32 vtkmNotUsed(_zoom),
                vtkm::Int32 subsetWidth,
                vtkm::Int32 minx,
                vtkm::Int32 miny,
                const vtkm::rendering::Camera& camera)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
  {
    vtkm::Float32 left, right, bottom, top;
    camera.GetViewRange2D(left, right, bottom, top);

    vtkm::Float32 vl, vr, vb, vt;
    camera.GetRealViewport(width, height, vl, vr, vb, vt);
    vtkm::Float32 _w = static_cast<vtkm::Float32>(width) * (vr - vl) / 2.f;
    vtkm::Float32 _h = static_cast<vtkm::Float32>(height) * (vt - vb) / 2.f;
    vtkm::Vec2f_32 minPoint(left, bottom);
    vtkm::Vec2f_32 maxPoint(right, top);
    vtkm::Vec2f_32 delta = maxPoint - minPoint;
    //delta[0] /= vtkm::Float32(width);
    //delta[1] /= vtkm::Float32(height);
    delta[0] /= vtkm::Float32(_w);
    delta[1] /= vtkm::Float32(_h);
    PixelDelta[0] = delta[0];
    PixelDelta[1] = delta[1];
    PixelDelta[2] = 0.f;

    vtkm::Vec2f_32 startOffset = minPoint + delta / 2.f;
    StartOffset[0] = startOffset[0];
    StartOffset[1] = startOffset[1];
    // always push the rays back from the origin
    StartOffset[2] = -1.f;


    vtkm::Normalize(nlook);
  }

  using ControlSignature =
    void(FieldOut, FieldOut, FieldOut, FieldOut, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6, _7);
  template <typename Precision>
  VTKM_EXEC void operator()(vtkm::Id idx,
                            Precision& rayDirX,
                            Precision& rayDirY,
                            Precision& rayDirZ,
                            Precision& rayOriginX,
                            Precision& rayOriginY,
                            Precision& rayOriginZ,
                            vtkm::Id& pixelIndex) const
  {
    // this is 2d so always look down z
    rayDirX = 0.f;
    rayDirY = 0.f;
    rayDirZ = 1.f;
    //
    // Pixel subset is the pixels in the 2d viewport
    // not where the rays might intersect data like
    // the perspective ray gen
    //
    int i = vtkm::Int32(idx) % SubsetWidth;
    int j = vtkm::Int32(idx) / SubsetWidth;

    vtkm::Vec3f_32 pos;
    pos[0] = vtkm::Float32(i);
    pos[1] = vtkm::Float32(j);
    pos[2] = 0.f;
    vtkm::Vec3f_32 origin = StartOffset + pos * PixelDelta;
    rayOriginX = origin[0];
    rayOriginY = origin[1];
    rayOriginZ = origin[2];
    i += Minx;
    j += Miny;
    pixelIndex = static_cast<vtkm::Id>(j * w + i);
  }

}; // class perspective ray gen

class Camera::PerspectiveRayGen : public vtkm::worklet::WorkletMapField
{
public:
  vtkm::Int32 w;
  vtkm::Int32 h;
  vtkm::Int32 Minx;
  vtkm::Int32 Miny;
  vtkm::Int32 SubsetWidth;
  vtkm::Vec3f_32 nlook; // normalized look
  vtkm::Vec3f_32 delta_x;
  vtkm::Vec3f_32 delta_y;
  VTKM_CONT
  PerspectiveRayGen(vtkm::Int32 width,
                    vtkm::Int32 height,
                    vtkm::Float32 fovX,
                    vtkm::Float32 fovY,
                    vtkm::Vec3f_32 look,
                    vtkm::Vec3f_32 up,
                    vtkm::Float32 _zoom,
                    vtkm::Int32 subsetWidth,
                    vtkm::Int32 minx,
                    vtkm::Int32 miny)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
  {
    vtkm::Float32 thx = tanf((fovX * vtkm::Pi_180f()) * .5f);
    vtkm::Float32 thy = tanf((fovY * vtkm::Pi_180f()) * .5f);
    vtkm::Vec3f_32 ru = vtkm::Cross(look, up);
    vtkm::Normalize(ru);

    vtkm::Vec3f_32 rv = vtkm::Cross(ru, look);
    vtkm::Normalize(rv);
    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    vtkm::Normalize(nlook);
  }

  using ControlSignature = void(FieldOut, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);
  template <typename Precision>
  VTKM_EXEC void operator()(vtkm::Id idx,
                            Precision& rayDirX,
                            Precision& rayDirY,
                            Precision& rayDirZ,
                            vtkm::Id& pixelIndex) const
  {
    vtkm::Vec<Precision, 3> ray_dir(rayDirX, rayDirY, rayDirZ);
    int i = vtkm::Int32(idx) % SubsetWidth;
    int j = vtkm::Int32(idx) / SubsetWidth;
    i += Minx;
    j += Miny;
    // Write out the global pixelId
    pixelIndex = static_cast<vtkm::Id>(j * w + i);
    ray_dir = nlook + delta_x * ((2.f * Precision(i) - Precision(w)) / 2.0f) +
      delta_y * ((2.f * Precision(j) - Precision(h)) / 2.0f);
    // avoid some numerical issues
    for (vtkm::Int32 d = 0; d < 3; ++d)
    {
      if (ray_dir[d] == 0.f)
        ray_dir[d] += 0.0000001f;
    }
    Precision dot = vtkm::Dot(ray_dir, ray_dir);
    Precision sq_mag = vtkm::Sqrt(dot);

    rayDirX = ray_dir[0] / sq_mag;
    rayDirY = ray_dir[1] / sq_mag;
    rayDirZ = ray_dir[2] / sq_mag;
  }

}; // class perspective ray gen

bool Camera::operator==(const Camera& other) const
{

  if (this->Height != other.Height)
    return false;
  if (this->Width != other.Width)
    return false;
  if (this->SubsetWidth != other.SubsetWidth)
    return false;
  if (this->SubsetHeight != other.SubsetHeight)
    return false;
  if (this->SubsetMinX != other.SubsetMinX)
    return false;
  if (this->SubsetMinY != other.SubsetMinY)
    return false;
  if (this->FovY != other.FovY)
    return false;
  if (this->FovX != other.FovX)
    return false;
  if (this->Zoom != other.Zoom)
    return false;
  if (this->Look[0] != other.Look[0])
    return false;
  if (this->Look[1] != other.Look[1])
    return false;
  if (this->Look[2] != other.Look[2])
    return false;
  if (this->LookAt[0] != other.LookAt[0])
    return false;
  if (this->LookAt[1] != other.LookAt[1])
    return false;
  if (this->LookAt[2] != other.LookAt[2])
    return false;
  if (this->Up[0] != other.Up[0])
    return false;
  if (this->Up[1] != other.Up[1])
    return false;
  if (this->Up[2] != other.Up[2])
    return false;
  if (this->Position[0] != other.Position[0])
    return false;
  if (this->Position[1] != other.Position[1])
    return false;
  if (this->Position[2] != other.Position[2])
    return false;
  return true;
}


VTKM_CONT
Camera::Camera()
{
  this->Height = 500;
  this->Width = 500;
  this->SubsetWidth = 500;
  this->SubsetHeight = 500;
  this->SubsetMinX = 0;
  this->SubsetMinY = 0;
  this->FovY = 30.f;
  this->FovX = 30.f;
  this->Zoom = 1.f;
  this->Look[0] = 0.f;
  this->Look[1] = 0.f;
  this->Look[2] = -1.f;
  this->LookAt[0] = 0.f;
  this->LookAt[1] = 0.f;
  this->LookAt[2] = -1.f;
  this->Up[0] = 0.f;
  this->Up[1] = 1.f;
  this->Up[2] = 0.f;
  this->Position[0] = 0.f;
  this->Position[1] = 0.f;
  this->Position[2] = 0.f;
  this->IsViewDirty = true;
}

VTKM_CONT
Camera::~Camera()
{
}

VTKM_CONT
void Camera::SetParameters(const vtkm::rendering::Camera& camera,
                           vtkm::rendering::CanvasRayTracer& canvas)
{
  this->SetUp(camera.GetViewUp());
  this->SetLookAt(camera.GetLookAt());
  this->SetPosition(camera.GetPosition());
  this->SetZoom(camera.GetZoom());
  this->SetFieldOfView(camera.GetFieldOfView());
  this->SetHeight(static_cast<vtkm::Int32>(canvas.GetHeight()));
  this->SetWidth(static_cast<vtkm::Int32>(canvas.GetWidth()));
  this->CameraView = camera;
  Canvas = canvas;
}


VTKM_CONT
void Camera::SetHeight(const vtkm::Int32& height)
{
  if (height <= 0)
  {
    throw vtkm::cont::ErrorBadValue("Camera height must be greater than zero.");
  }
  if (Height != height)
  {
    this->Height = height;
    this->SetFieldOfView(this->FovY);
  }
}

VTKM_CONT
vtkm::Int32 Camera::GetHeight() const
{
  return this->Height;
}

VTKM_CONT
void Camera::SetWidth(const vtkm::Int32& width)
{
  if (width <= 0)
  {
    throw vtkm::cont::ErrorBadValue("Camera width must be greater than zero.");
  }
  if (this->Width != width)
  {
    this->Width = width;
    this->SetFieldOfView(this->FovY);
  }
}

VTKM_CONT
vtkm::Int32 Camera::GetWidth() const
{
  return this->Width;
}

VTKM_CONT
vtkm::Int32 Camera::GetSubsetWidth() const
{
  return this->SubsetWidth;
}

VTKM_CONT
vtkm::Int32 Camera::GetSubsetHeight() const
{
  return this->SubsetHeight;
}

VTKM_CONT
void Camera::SetZoom(const vtkm::Float32& zoom)
{
  if (zoom <= 0)
  {
    throw vtkm::cont::ErrorBadValue("Camera zoom must be greater than zero.");
  }
  if (this->Zoom != zoom)
  {
    this->IsViewDirty = true;
    this->Zoom = zoom;
  }
}

VTKM_CONT
vtkm::Float32 Camera::GetZoom() const
{
  return this->Zoom;
}

VTKM_CONT
void Camera::SetFieldOfView(const vtkm::Float32& degrees)
{
  if (degrees <= 0)
  {
    throw vtkm::cont::ErrorBadValue("Camera feild of view must be greater than zero.");
  }
  if (degrees > 180)
  {
    throw vtkm::cont::ErrorBadValue("Camera feild of view must be less than 180.");
  }

  vtkm::Float32 newFOVY = degrees;
  vtkm::Float32 newFOVX;

  if (this->Width != this->Height)
  {
    vtkm::Float32 fovyRad = newFOVY * vtkm::Pi_180f();

    // Use the tan function to find the distance from the center of the image to the top (or
    // bottom). (Actually, we are finding the ratio of this distance to the near plane distance,
    // but since we scale everything by the near plane distance, we can use this ratio as a scaled
    // proxy of the distances we need.)
    vtkm::Float32 verticalDistance = vtkm::Tan(0.5f * fovyRad);

    // Scale the vertical distance by the aspect ratio to get the horizontal distance.
    vtkm::Float32 aspectRatio = vtkm::Float32(this->Width) / vtkm::Float32(this->Height);
    vtkm::Float32 horizontalDistance = aspectRatio * verticalDistance;

    // Now use the arctan function to get the proper field of view in the x direction.
    vtkm::Float32 fovxRad = 2.0f * vtkm::ATan(horizontalDistance);
    newFOVX = fovxRad / vtkm::Pi_180f();
  }
  else
  {
    newFOVX = newFOVY;
  }

  if (newFOVX != this->FovX)
  {
    this->IsViewDirty = true;
  }
  if (newFOVY != this->FovY)
  {
    this->IsViewDirty = true;
  }
  this->FovX = newFOVX;
  this->FovY = newFOVY;
  this->CameraView.SetFieldOfView(this->FovY);
}

VTKM_CONT
vtkm::Float32 Camera::GetFieldOfView() const
{
  return this->FovY;
}

VTKM_CONT
void Camera::SetUp(const vtkm::Vec3f_32& up)
{
  if (this->Up != up)
  {
    this->Up = up;
    vtkm::Normalize(this->Up);
    this->IsViewDirty = true;
  }
}

VTKM_CONT
vtkm::Vec3f_32 Camera::GetUp() const
{
  return this->Up;
}

VTKM_CONT
void Camera::SetLookAt(const vtkm::Vec3f_32& lookAt)
{
  if (this->LookAt != lookAt)
  {
    this->LookAt = lookAt;
    this->IsViewDirty = true;
  }
}

VTKM_CONT
vtkm::Vec3f_32 Camera::GetLookAt() const
{
  return this->LookAt;
}

VTKM_CONT
void Camera::SetPosition(const vtkm::Vec3f_32& position)
{
  if (this->Position != position)
  {
    this->Position = position;
    this->IsViewDirty = true;
  }
}

VTKM_CONT
vtkm::Vec3f_32 Camera::GetPosition() const
{
  return this->Position;
}

VTKM_CONT
void Camera::ResetIsViewDirty()
{
  this->IsViewDirty = false;
}

VTKM_CONT
bool Camera::GetIsViewDirty() const
{
  return this->IsViewDirty;
}

void Camera::GetPixelData(const vtkm::cont::CoordinateSystem& coords,
                          vtkm::Int32& activePixels,
                          vtkm::Float32& aveRayDistance)
{
  vtkm::Bounds boundingBox = coords.GetBounds();
  this->FindSubset(boundingBox);
  //Reset the camera look vector
  this->Look = this->LookAt - this->Position;
  vtkm::Normalize(this->Look);
  const int size = this->SubsetWidth * this->SubsetHeight;
  vtkm::cont::ArrayHandle<vtkm::Float32> dists;
  vtkm::cont::ArrayHandle<vtkm::Int32> hits;
  dists.Allocate(size);
  hits.Allocate(size);

  //Create the ray direction
  vtkm::worklet::DispatcherMapField<PixelData>(PixelData(this->Width,
                                                         this->Height,
                                                         this->FovX,
                                                         this->FovY,
                                                         this->Look,
                                                         this->Up,
                                                         this->Zoom,
                                                         this->SubsetWidth,
                                                         this->SubsetMinX,
                                                         this->SubsetMinY,
                                                         this->Position,
                                                         boundingBox))
    .Invoke(hits, dists); //X Y Z
  activePixels = vtkm::cont::Algorithm::Reduce(hits, vtkm::Int32(0));
  aveRayDistance =
    vtkm::cont::Algorithm::Reduce(dists, vtkm::Float32(0)) / vtkm::Float32(activePixels);
}

VTKM_CONT
void Camera::CreateRays(Ray<vtkm::Float32>& rays, vtkm::Bounds bounds)
{
  CreateRaysImpl(rays, bounds);
}

VTKM_CONT
void Camera::CreateRays(Ray<vtkm::Float64>& rays, vtkm::Bounds bounds)
{
  CreateRaysImpl(rays, bounds);
}

template <typename Precision>
VTKM_CONT void Camera::CreateRaysImpl(Ray<Precision>& rays, const vtkm::Bounds boundingBox)
{
  Logger* logger = Logger::GetInstance();
  vtkm::cont::Timer createTimer;
  createTimer.Start();
  logger->OpenLogEntry("ray_camera");

  bool ortho = this->CameraView.GetMode() == vtkm::rendering::Camera::MODE_2D;
  this->UpdateDimensions(rays, boundingBox, ortho);
  this->WriteSettingsToLog();
  vtkm::cont::Timer timer;
  timer.Start();
  //Set the origin of the ray back to the camera position

  Precision infinity;
  GetInfinity(infinity);

  vtkm::cont::ArrayHandleConstant<Precision> inf(infinity, rays.NumRays);
  vtkm::cont::Algorithm::Copy(inf, rays.MaxDistance);

  vtkm::cont::ArrayHandleConstant<Precision> zero(0, rays.NumRays);
  vtkm::cont::Algorithm::Copy(zero, rays.MinDistance);
  vtkm::cont::Algorithm::Copy(zero, rays.Distance);

  vtkm::cont::ArrayHandleConstant<vtkm::Id> initHit(-2, rays.NumRays);
  vtkm::cont::Algorithm::Copy(initHit, rays.HitIdx);

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("camera_memset", time);
  timer.Start();

  //Reset the camera look vector
  this->Look = this->LookAt - this->Position;
  vtkm::Normalize(this->Look);
  if (ortho)
  {

    vtkm::worklet::DispatcherMapField<Ortho2DRayGen> dispatcher(Ortho2DRayGen(this->Width,
                                                                              this->Height,
                                                                              this->Zoom,
                                                                              this->SubsetWidth,
                                                                              this->SubsetMinX,
                                                                              this->SubsetMinY,
                                                                              this->CameraView));
    dispatcher.Invoke(rays.DirX,
                      rays.DirY,
                      rays.DirZ,
                      rays.OriginX,
                      rays.OriginY,
                      rays.OriginZ,
                      rays.PixelIdx); //X Y Z
  }
  else
  {
    //Create the ray direction
    vtkm::worklet::DispatcherMapField<PerspectiveRayGen> dispatcher(
      PerspectiveRayGen(this->Width,
                        this->Height,
                        this->FovX,
                        this->FovY,
                        this->Look,
                        this->Up,
                        this->Zoom,
                        this->SubsetWidth,
                        this->SubsetMinX,
                        this->SubsetMinY));
    dispatcher.Invoke(rays.DirX, rays.DirY, rays.DirZ, rays.PixelIdx); //X Y Z

    vtkm::cont::ArrayHandleConstant<Precision> posX(this->Position[0], rays.NumRays);
    vtkm::cont::Algorithm::Copy(posX, rays.OriginX);

    vtkm::cont::ArrayHandleConstant<Precision> posY(this->Position[1], rays.NumRays);
    vtkm::cont::Algorithm::Copy(posY, rays.OriginY);

    vtkm::cont::ArrayHandleConstant<Precision> posZ(this->Position[2], rays.NumRays);
    vtkm::cont::Algorithm::Copy(posZ, rays.OriginZ);
  }

  time = timer.GetElapsedTime();
  logger->AddLogData("ray_gen", time);
  time = createTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} //create rays

VTKM_CONT
void Camera::FindSubset(const vtkm::Bounds& bounds)
{
  this->ViewProjectionMat =
    vtkm::MatrixMultiply(this->CameraView.CreateProjectionMatrix(this->Width, this->Height),
                         this->CameraView.CreateViewMatrix());
  vtkm::Float32 x[2], y[2], z[2];
  x[0] = static_cast<vtkm::Float32>(bounds.X.Min);
  x[1] = static_cast<vtkm::Float32>(bounds.X.Max);
  y[0] = static_cast<vtkm::Float32>(bounds.Y.Min);
  y[1] = static_cast<vtkm::Float32>(bounds.Y.Max);
  z[0] = static_cast<vtkm::Float32>(bounds.Z.Min);
  z[1] = static_cast<vtkm::Float32>(bounds.Z.Max);
  //Inise the data bounds
  if (this->Position[0] >= x[0] && this->Position[0] <= x[1] && this->Position[1] >= y[0] &&
      this->Position[1] <= y[1] && this->Position[2] >= z[0] && this->Position[2] <= z[1])
  {
    this->SubsetWidth = this->Width;
    this->SubsetHeight = this->Height;
    this->SubsetMinY = 0;
    this->SubsetMinX = 0;
    return;
  }

  vtkm::Float32 xmin, ymin, xmax, ymax, zmin, zmax;
  xmin = vtkm::Infinity32();
  ymin = vtkm::Infinity32();
  zmin = vtkm::Infinity32();
  xmax = vtkm::NegativeInfinity32();
  ymax = vtkm::NegativeInfinity32();
  zmax = vtkm::NegativeInfinity32();
  vtkm::Vec4f_32 extentPoint;
  for (vtkm::Int32 i = 0; i < 2; ++i)
    for (vtkm::Int32 j = 0; j < 2; ++j)
      for (vtkm::Int32 k = 0; k < 2; ++k)
      {
        extentPoint[0] = x[i];
        extentPoint[1] = y[j];
        extentPoint[2] = z[k];
        extentPoint[3] = 1.f;
        vtkm::Vec4f_32 transformed = vtkm::MatrixMultiply(this->ViewProjectionMat, extentPoint);
        // perform the perspective divide
        for (vtkm::Int32 a = 0; a < 3; ++a)
        {
          transformed[a] = transformed[a] / transformed[3];
        }

        transformed[0] = (transformed[0] * 0.5f + 0.5f) * static_cast<vtkm::Float32>(Width);
        transformed[1] = (transformed[1] * 0.5f + 0.5f) * static_cast<vtkm::Float32>(Height);
        transformed[2] = (transformed[2] * 0.5f + 0.5f);
        zmin = vtkm::Min(zmin, transformed[2]);
        zmax = vtkm::Max(zmax, transformed[2]);
        if (transformed[2] < 0 || transformed[2] > 1)
        {
          continue;
        }
        xmin = vtkm::Min(xmin, transformed[0]);
        ymin = vtkm::Min(ymin, transformed[1]);
        xmax = vtkm::Max(xmax, transformed[0]);
        ymax = vtkm::Max(ymax, transformed[1]);
      }

  xmin -= .001f;
  xmax += .001f;
  ymin -= .001f;
  ymax += .001f;
  xmin = vtkm::Floor(vtkm::Min(vtkm::Max(0.f, xmin), vtkm::Float32(Width)));
  xmax = vtkm::Ceil(vtkm::Min(vtkm::Max(0.f, xmax), vtkm::Float32(Width)));
  ymin = vtkm::Floor(vtkm::Min(vtkm::Max(0.f, ymin), vtkm::Float32(Height)));
  ymax = vtkm::Ceil(vtkm::Min(vtkm::Max(0.f, ymax), vtkm::Float32(Height)));

  Logger* logger = Logger::GetInstance();
  std::stringstream ss;
  ss << "(" << xmin << "," << ymin << "," << zmin << ")-";
  ss << "(" << xmax << "," << ymax << "," << zmax << ")";
  logger->AddLogData("pixel_range", ss.str());

  vtkm::Int32 dx = vtkm::Int32(xmax) - vtkm::Int32(xmin);
  vtkm::Int32 dy = vtkm::Int32(ymax) - vtkm::Int32(ymin);
  //
  //  scene is behind the camera
  //
  if (zmax < 0 || xmin >= xmax || ymin >= ymax)
  {
    this->SubsetWidth = 1;
    this->SubsetHeight = 1;
    this->SubsetMinX = 0;
    this->SubsetMinY = 0;
  }
  else
  {
    this->SubsetWidth = dx;
    this->SubsetHeight = dy;
    this->SubsetMinX = vtkm::Int32(xmin);
    this->SubsetMinY = vtkm::Int32(ymin);
  }
  logger->AddLogData("subset_width", dx);
  logger->AddLogData("subset_height", dy);
}

template <typename Precision>
VTKM_CONT void Camera::UpdateDimensions(Ray<Precision>& rays,
                                        const vtkm::Bounds& boundingBox,
                                        bool ortho2D)
{
  // If bounds have been provided, only cast rays that could hit the data
  bool imageSubsetModeOn = boundingBox.IsNonEmpty();

  //Find the pixel footprint
  if (imageSubsetModeOn && !ortho2D)
  {
    //Create a transform matrix using the rendering::camera class
    vtkm::rendering::Camera camera = this->CameraView;
    camera.SetFieldOfView(this->GetFieldOfView());
    camera.SetLookAt(this->GetLookAt());
    camera.SetPosition(this->GetPosition());
    camera.SetViewUp(this->GetUp());
    //
    // Just create come clipping range, we ignore the zmax value in subsetting
    //
    vtkm::Float64 maxDim = vtkm::Max(
      boundingBox.X.Max - boundingBox.X.Min,
      vtkm::Max(boundingBox.Y.Max - boundingBox.Y.Min, boundingBox.Z.Max - boundingBox.Z.Min));

    maxDim *= 100;
    camera.SetClippingRange(.0001, maxDim);
    //Update our ViewProjection matrix
    this->ViewProjectionMat =
      vtkm::MatrixMultiply(this->CameraView.CreateProjectionMatrix(this->Width, this->Height),
                           this->CameraView.CreateViewMatrix());
    this->FindSubset(boundingBox);
  }
  else if (ortho2D)
  {
    // 2D rendering has a viewport that represents the area of the canvas where the image
    // is drawn. Thus, we have to create rays cooresponding to that region of the
    // canvas, so annotations are correctly rendered
    vtkm::Float32 vl, vr, vb, vt;
    this->CameraView.GetRealViewport(this->GetWidth(), this->GetHeight(), vl, vr, vb, vt);
    vtkm::Float32 _x = static_cast<vtkm::Float32>(this->GetWidth()) * (1.f + vl) / 2.f;
    vtkm::Float32 _y = static_cast<vtkm::Float32>(this->GetHeight()) * (1.f + vb) / 2.f;
    vtkm::Float32 _w = static_cast<vtkm::Float32>(this->GetWidth()) * (vr - vl) / 2.f;
    vtkm::Float32 _h = static_cast<vtkm::Float32>(this->GetHeight()) * (vt - vb) / 2.f;

    this->SubsetWidth = static_cast<vtkm::Int32>(_w);
    this->SubsetHeight = static_cast<vtkm::Int32>(_h);
    this->SubsetMinY = static_cast<vtkm::Int32>(_y);
    this->SubsetMinX = static_cast<vtkm::Int32>(_x);
  }
  else
  {
    //Update the image dimensions
    this->SubsetWidth = this->Width;
    this->SubsetHeight = this->Height;
    this->SubsetMinY = 0;
    this->SubsetMinX = 0;
  }

  // resize rays and buffers
  if (rays.NumRays != SubsetWidth * SubsetHeight)
  {
    RayOperations::Resize(
      rays, this->SubsetHeight * this->SubsetWidth, vtkm::cont::DeviceAdapterTagSerial());
  }
}

void Camera::CreateDebugRay(vtkm::Vec2i_32 pixel, Ray<vtkm::Float64>& rays)
{
  CreateDebugRayImp(pixel, rays);
}

void Camera::CreateDebugRay(vtkm::Vec2i_32 pixel, Ray<vtkm::Float32>& rays)
{
  CreateDebugRayImp(pixel, rays);
}

template <typename Precision>
void Camera::CreateDebugRayImp(vtkm::Vec2i_32 pixel, Ray<Precision>& rays)
{
  RayOperations::Resize(rays, 1, vtkm::cont::DeviceAdapterTagSerial());
  vtkm::Int32 pixelIndex = this->Width * (this->Height - pixel[1]) + pixel[0];
  rays.PixelIdx.GetPortalControl().Set(0, pixelIndex);
  rays.OriginX.GetPortalControl().Set(0, this->Position[0]);
  rays.OriginY.GetPortalControl().Set(0, this->Position[1]);
  rays.OriginZ.GetPortalControl().Set(0, this->Position[2]);


  vtkm::Float32 infinity;
  GetInfinity(infinity);

  rays.MaxDistance.GetPortalControl().Set(0, infinity);
  rays.MinDistance.GetPortalControl().Set(0, 0.f);
  rays.HitIdx.GetPortalControl().Set(0, -2);

  vtkm::Float32 thx = tanf((this->FovX * vtkm::Pi_180f()) * .5f);
  vtkm::Float32 thy = tanf((this->FovY * vtkm::Pi_180f()) * .5f);
  vtkm::Vec3f_32 ru = vtkm::Cross(this->Look, this->Up);
  vtkm::Normalize(ru);

  vtkm::Vec3f_32 rv = vtkm::Cross(ru, this->Look);
  vtkm::Vec3f_32 delta_x, delta_y;
  vtkm::Normalize(rv);
  delta_x = ru * (2 * thx / (float)this->Width);
  delta_y = rv * (2 * thy / (float)this->Height);

  if (this->Zoom > 0)
  {
    vtkm::Float32 _zoom = this->Zoom;
    delta_x[0] = delta_x[0] / _zoom;
    delta_x[1] = delta_x[1] / _zoom;
    delta_x[2] = delta_x[2] / _zoom;
    delta_y[0] = delta_y[0] / _zoom;
    delta_y[1] = delta_y[1] / _zoom;
    delta_y[2] = delta_y[2] / _zoom;
  }
  vtkm::Vec3f_32 nlook = this->Look;
  vtkm::Normalize(nlook);

  vtkm::Vec<Precision, 3> ray_dir;
  int i = vtkm::Int32(pixelIndex) % this->Width;
  int j = vtkm::Int32(pixelIndex) / this->Height;
  ray_dir = nlook + delta_x * ((2.f * Precision(i) - Precision(this->Width)) / 2.0f) +
    delta_y * ((2.f * Precision(j) - Precision(this->Height)) / 2.0f);

  Precision dot = vtkm::Dot(ray_dir, ray_dir);
  Precision sq_mag = vtkm::Sqrt(dot);

  ray_dir[0] = ray_dir[0] / sq_mag;
  ray_dir[1] = ray_dir[1] / sq_mag;
  ray_dir[2] = ray_dir[2] / sq_mag;
  rays.DirX.GetPortalControl().Set(0, ray_dir[0]);
  rays.DirY.GetPortalControl().Set(0, ray_dir[1]);
  rays.DirZ.GetPortalControl().Set(0, ray_dir[2]);
}

void Camera::WriteSettingsToLog()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("position_x", Position[0]);
  logger->AddLogData("position_y", Position[1]);
  logger->AddLogData("position_z", Position[2]);

  logger->AddLogData("lookat_x", LookAt[0]);
  logger->AddLogData("lookat_y", LookAt[1]);
  logger->AddLogData("lookat_z", LookAt[2]);

  logger->AddLogData("up_x", Up[0]);
  logger->AddLogData("up_y", Up[1]);
  logger->AddLogData("up_z", Up[2]);

  logger->AddLogData("fov_x", FovX);
  logger->AddLogData("fov_y", FovY);
  logger->AddLogData("width", Width);
  logger->AddLogData("height", Height);
  logger->AddLogData("subset_height", SubsetHeight);
  logger->AddLogData("subset_width", SubsetWidth);
  logger->AddLogData("num_rays", SubsetWidth * SubsetHeight);
}

std::string Camera::ToString()
{
  std::stringstream sstream;
  sstream << "------------------------------------------------------------\n";
  sstream << "Position : [" << this->Position[0] << ",";
  sstream << this->Position[1] << ",";
  sstream << this->Position[2] << "]\n";
  sstream << "LookAt   : [" << this->LookAt[0] << ",";
  sstream << this->LookAt[1] << ",";
  sstream << this->LookAt[2] << "]\n";
  sstream << "FOV_X    : " << this->FovX << "\n";
  sstream << "Up       : [" << this->Up[0] << ",";
  sstream << this->Up[1] << ",";
  sstream << this->Up[2] << "]\n";
  sstream << "Width    : " << this->Width << "\n";
  sstream << "Height   : " << this->Height << "\n";
  sstream << "------------------------------------------------------------\n";
  return sstream.str();
}
}
}
} //namespace vtkm::rendering::raytracing
