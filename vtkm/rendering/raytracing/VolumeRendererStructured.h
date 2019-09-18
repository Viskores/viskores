//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_VolumeRendererStructured_h
#define vtk_m_rendering_raytracing_VolumeRendererStructured_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/rendering/raytracing/Ray.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VolumeRendererStructured
{
public:
  using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using CartesianArrayHandle =
    vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

  VTKM_CONT
  VolumeRendererStructured();

  VTKM_CONT
  void EnableCompositeBackground();

  VTKM_CONT
  void DisableCompositeBackground();

  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap);

  VTKM_CONT
  void SetData(const vtkm::cont::CoordinateSystem& coords,
               const vtkm::cont::Field& scalarField,
               const vtkm::cont::CellSetStructured<3>& cellset,
               const vtkm::Range& scalarRange);


  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);
  //VTKM_CONT
  ///void Render(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays);


  VTKM_CONT
  void SetSampleDistance(const vtkm::Float32& distance);

protected:
  template <typename Precision, typename Device>
  VTKM_CONT void RenderOnDevice(vtkm::rendering::raytracing::Ray<Precision>& rays, Device);
  template <typename Precision>
  struct RenderFunctor;

  bool IsSceneDirty;
  bool IsUniformDataSet;
  vtkm::Bounds SpatialExtent;
  vtkm::cont::ArrayHandleVirtualCoordinates Coordinates;
  vtkm::cont::CellSetStructured<3> Cellset;
  const vtkm::cont::Field* ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> ColorMap;
  vtkm::Float32 SampleDistance;
  vtkm::Range ScalarRange;
};
}
}
} //namespace vtkm::rendering::raytracing
#endif
