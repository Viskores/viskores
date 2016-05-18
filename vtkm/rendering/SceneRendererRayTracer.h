//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_SceneRendererRayTracer_h
#define vtk_m_rendering_SceneRendererRayTracer_h
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/DeviceAdapterTagSerial.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/Triangulator.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/RenderSurfaceRayTracer.h>
#include <vtkm/rendering/View.h>
namespace vtkm {
namespace rendering {

//  static bool doOnce = true;
template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class SceneRendererRayTracer : public SceneRenderer
{ 
protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
  vtkm::rendering::raytracing::RayTracer<DeviceAdapter> Tracer;
  RenderSurfaceRayTracer *Surface;
public:
  VTKM_CONT_EXPORT
  SceneRendererRayTracer()
  {
    Surface = NULL;
  }
  VTKM_CONT_EXPORT
  void SetRenderSurface(RenderSurface *surface)
  {
    if(surface != NULL) 
    {
  
      Surface = dynamic_cast<RenderSurfaceRayTracer*>(surface);
      if(Surface == NULL)
      {
        throw vtkm::cont::ErrorControlBadValue(
          "Ray Tracer: bad surface type. Must be RenderSurfaceRayTracer");
      }
    }
  }
  VTKM_CONT_EXPORT
  void SetActiveColorTable(const ColorTable &colorTable)
  {
    colorTable.Sample(1024, ColorMap);
  }

  VTKM_CONT_EXPORT
  void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                   const vtkm::cont::CoordinateSystem &coords,
                   vtkm::cont::Field &scalarField,
                   const vtkm::rendering::ColorTable &colorTable,
                   vtkm::rendering::View &view,                                      
                   vtkm::Float64 *scalarBounds)
  {
    
    vtkm::cont::Timer<DeviceAdapter> timer;
    const vtkm::cont::DynamicArrayHandleCoordinateSystem dynamicCoordsHandle = coords.GetData();
    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  indices;
    vtkm::rendering::raytracing::Camera<DeviceAdapter> &camera = Tracer.GetCamera();
    camera.SetParameters(view);
    vtkm::Id numberOfTriangles;

    vtkm::Float64 dataBounds[6];
    coords.GetBounds(dataBounds,DeviceAdapter());

    Triangulator<DeviceAdapter> triangulator;
    triangulator.run(cellset, indices, numberOfTriangles);//,dynamicCoordsHandle,dataBounds);
    
    Tracer.SetData(dynamicCoordsHandle, indices, scalarField, numberOfTriangles, scalarBounds, dataBounds);
    Tracer.SetColorMap(ColorMap);
    Tracer.SetBackgroundColor(BackgroundColor);
    Tracer.Render(Surface);
  }
};
}} //namespace vtkm::rendering
#endif //vtk_m_rendering_SceneRendererRayTracer_h
