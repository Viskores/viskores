//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Camera_h
#define vtk_m_rendering_raytracing_Camera_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/raytracing/Ray.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT Camera
{
private:
  vtkm::Int32 Height = 500;
  vtkm::Int32 Width = 500;
  vtkm::Int32 SubsetWidth = 500;
  vtkm::Int32 SubsetHeight = 500;
  vtkm::Int32 SubsetMinX = 0;
  vtkm::Int32 SubsetMinY = 0;
  vtkm::Float32 FovX = 30.f;
  vtkm::Float32 FovY = 30.f;
  vtkm::Float32 Zoom = 1.f;
  bool IsViewDirty = true;

  vtkm::Vec3f_32 Look{ 0.f, 0.f, -1.f };
  vtkm::Vec3f_32 Up{ 0.f, 1.f, 0.f };
  vtkm::Vec3f_32 LookAt{ 0.f, 0.f, -1.f };
  vtkm::Vec3f_32 Position{ 0.f, 0.f, 0.f };
  vtkm::rendering::Camera CameraView;
  vtkm::Matrix<vtkm::Float32, 4, 4> ViewProjectionMat;

public:
  VTKM_CONT
  std::string ToString();

  VTKM_CONT
  void SetParameters(const vtkm::rendering::Camera& camera, vtkm::Int32 width, vtkm::Int32 height);

  VTKM_CONT
  void SetHeight(const vtkm::Int32& height);

  VTKM_CONT
  vtkm::Int32 GetHeight() const;

  VTKM_CONT
  void SetWidth(const vtkm::Int32& width);

  VTKM_CONT
  vtkm::Int32 GetWidth() const;

  VTKM_CONT
  vtkm::Int32 GetSubsetWidth() const;

  VTKM_CONT
  vtkm::Int32 GetSubsetHeight() const;

  VTKM_CONT
  void SetZoom(const vtkm::Float32& zoom);

  VTKM_CONT
  vtkm::Float32 GetZoom() const;

  VTKM_CONT
  void SetFieldOfView(const vtkm::Float32& degrees);

  VTKM_CONT
  vtkm::Float32 GetFieldOfView() const;

  VTKM_CONT
  void SetUp(const vtkm::Vec3f_32& up);

  VTKM_CONT
  void SetPosition(const vtkm::Vec3f_32& position);

  VTKM_CONT
  vtkm::Vec3f_32 GetPosition() const;

  VTKM_CONT
  vtkm::Vec3f_32 GetUp() const;

  VTKM_CONT
  void SetLookAt(const vtkm::Vec3f_32& lookAt);

  VTKM_CONT
  vtkm::Vec3f_32 GetLookAt() const;

  VTKM_CONT
  void ResetIsViewDirty();

  VTKM_CONT
  bool GetIsViewDirty() const;

  VTKM_CONT
  void CreateRays(Ray<vtkm::Float32>& rays, const vtkm::Bounds& bounds);

  VTKM_CONT
  void CreateRays(Ray<vtkm::Float64>& rays, const vtkm::Bounds& bounds);

  VTKM_CONT
  void GetPixelData(const vtkm::cont::CoordinateSystem& coords,
                    vtkm::Int32& activePixels,
                    vtkm::Float32& aveRayDistance);

  template <typename Precision>
  VTKM_CONT void CreateRaysImpl(Ray<Precision>& rays, const vtkm::Bounds& boundingBox);

  void CreateDebugRay(vtkm::Vec2i_32 pixel, Ray<vtkm::Float32>& rays);

  void CreateDebugRay(vtkm::Vec2i_32 pixel, Ray<vtkm::Float64>& rays);

  bool operator==(const Camera& other) const;

private:
  template <typename Precision>
  VTKM_CONT void CreateDebugRayImp(vtkm::Vec2i_32 pixel, Ray<Precision>& rays);

  VTKM_CONT
  void FindSubset(const vtkm::Bounds& bounds);

  VTKM_CONT
  void WriteSettingsToLog();

  template <typename Precision>
  VTKM_CONT void UpdateDimensions(Ray<Precision>& rays,
                                  const vtkm::Bounds& boundingBox,
                                  bool ortho2D);

}; // class camera
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Camera_h
