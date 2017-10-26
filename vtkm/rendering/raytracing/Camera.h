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
#ifndef vtk_m_rendering_raytracing_Camera_h
#define vtk_m_rendering_raytracing_Camera_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
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
  struct PixelDataFunctor;
  template <typename Precision>
  struct CreateRaysFunctor;
  vtkm::rendering::CanvasRayTracer Canvas;
  vtkm::Int32 Height;
  vtkm::Int32 Width;
  vtkm::Int32 SubsetWidth;
  vtkm::Int32 SubsetHeight;
  vtkm::Int32 SubsetMinX;
  vtkm::Int32 SubsetMinY;
  vtkm::Float32 FovX;
  vtkm::Float32 FovY;
  vtkm::Float32 Zoom;
  bool IsViewDirty;

  vtkm::Vec<vtkm::Float32, 3> Look;
  vtkm::Vec<vtkm::Float32, 3> Up;
  vtkm::Vec<vtkm::Float32, 3> LookAt;
  vtkm::Vec<vtkm::Float32, 3> Position;
  vtkm::rendering::Camera CameraView;
  vtkm::Matrix<vtkm::Float32, 4, 4> ViewProjectionMat;

public:
  VTKM_CONT
  Camera();

  VTKM_CONT
  ~Camera();

  // cuda does not compile if this is private
  class PerspectiveRayGen;
  class Ortho2DRayGen;

  std::string ToString();

  VTKM_CONT
  void SetParameters(const vtkm::rendering::Camera& camera,
                     vtkm::rendering::CanvasRayTracer& canvas);


  VTKM_CONT
  void SetHeight(const vtkm::Int32& height);

  VTKM_CONT
  void WriteSettingsToLog();

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
  void SetUp(const vtkm::Vec<vtkm::Float32, 3>& up);

  VTKM_CONT
  void SetPosition(const vtkm::Vec<vtkm::Float32, 3>& position);

  VTKM_CONT
  vtkm::Vec<vtkm::Float32, 3> GetPosition() const;

  VTKM_CONT
  vtkm::Vec<vtkm::Float32, 3> GetUp() const;

  VTKM_CONT
  void SetLookAt(const vtkm::Vec<vtkm::Float32, 3>& lookAt);

  VTKM_CONT
  vtkm::Vec<vtkm::Float32, 3> GetLookAt() const;

  VTKM_CONT
  void ResetIsViewDirty();

  VTKM_CONT
  bool GetIsViewDirty() const;

  VTKM_CONT
  void CreateRays(Ray<vtkm::Float32>& rays, const vtkm::cont::CoordinateSystem& coords);
  VTKM_CONT
  void CreateRays(Ray<vtkm::Float64>& rays, const vtkm::cont::CoordinateSystem& coords);

  VTKM_CONT
  void GetPixelData(const vtkm::cont::CoordinateSystem& coords,
                    vtkm::Int32& activePixels,
                    vtkm::Float32& aveRayDistance);

  template <typename Precision, typename DeviceAdapter>
  VTKM_CONT void CreateRaysOnDevice(Ray<Precision>& rays,
                                    DeviceAdapter,
                                    const vtkm::Bounds boundingBox);

  void CreateDebugRay(vtkm::Vec<vtkm::Int32, 2> pixel, Ray<vtkm::Float32>& rays);

  void CreateDebugRay(vtkm::Vec<vtkm::Int32, 2> pixel, Ray<vtkm::Float64>& rays);

  bool operator==(const Camera& other) const;

private:
  template <typename Precision>
  void CreateDebugRayImp(vtkm::Vec<vtkm::Int32, 2> pixel, Ray<Precision>& rays);
  VTKM_CONT
  void FindSubset(const vtkm::Bounds& bounds);

  template <typename DeviceAdapter, typename Precision>
  VTKM_CONT void UpdateDimensions(Ray<Precision>& rays,
                                  DeviceAdapter,
                                  const vtkm::Bounds& boundingBox,
                                  bool ortho2D);

}; // class camera
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Camera_h
