//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_ConnectivityProxy_h
#define vtk_m_rendering_ConnectivityProxy_h

#include <memory>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/PartialComposite.h>
#include <vtkm/rendering/raytracing/Ray.h>

namespace vtkm
{
namespace rendering
{

using PartialVector64 = std::vector<vtkm::rendering::raytracing::PartialComposite<vtkm::Float64>>;
using PartialVector32 = std::vector<vtkm::rendering::raytracing::PartialComposite<vtkm::Float32>>;

class VTKM_RENDERING_EXPORT ConnectivityProxy
{
public:
  ConnectivityProxy(vtkm::cont::DataSet& dataset);
  ConnectivityProxy(const vtkm::cont::DynamicCellSet& cellset,
                    const vtkm::cont::CoordinateSystem& coords,
                    const vtkm::cont::Field& scalarField);
  ~ConnectivityProxy();
  enum RenderMode
  {
    VOLUME_MODE,
    ENERGY_MODE
  };

  void SetRenderMode(RenderMode mode);
  void SetSampleDistance(const vtkm::Float32&);
  void SetCanvas(vtkm::rendering::Canvas* canvas);
  void SetScalarField(const std::string& fieldName);
  void SetEmissionField(const std::string& fieldName);
  void SetCamera(const vtkm::rendering::Camera& camera);
  void SetScalarRange(const vtkm::Range& range);
  void SetColorMap(vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colormap);
  void SetCompositeBackground(bool on);
  void SetDebugPrints(bool on);
  void SetUnitScalar(vtkm::Float32 unitScalar);

  vtkm::Bounds GetSpatialBounds();
  vtkm::Range GetScalarFieldRange();

  void Trace(const vtkm::rendering::Camera& camera, vtkm::rendering::CanvasRayTracer* canvas);
  void Trace(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays);
  void Trace(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);

  PartialVector64 PartialTrace(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays);
  PartialVector32 PartialTrace(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);

protected:
  struct InternalsType;
  struct BoundsFunctor;
  std::shared_ptr<InternalsType> Internals;

private:
  // Do not allow the default constructor
  ConnectivityProxy();
};
}
} //namespace vtkm::rendering
#endif //vtk_m_rendering_SceneRendererVolume_h
