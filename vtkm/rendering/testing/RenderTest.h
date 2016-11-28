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
#ifndef vtk_m_rendering_testing_RenderTest_h
#define vtk_m_rendering_testing_RenderTest_h

#include <vtkm/Bounds.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View1D.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/cont/DeviceAdapter.h>

namespace vtkm {
namespace rendering {
namespace testing {

template <typename ViewType>
inline void
SetCamera(vtkm::rendering::Camera &camera,
          const vtkm::Bounds &coordBounds);
template <typename ViewType>
inline void
SetCamera(vtkm::rendering::Camera &camera,
          const vtkm::Bounds &coordBounds,
          const vtkm::cont::Field &field);

template <>
inline void
SetCamera<vtkm::rendering::View3D>(vtkm::rendering::Camera &camera,
                                   const vtkm::Bounds &coordBounds)
{
  camera = vtkm::rendering::Camera();
  camera.ResetToBounds(coordBounds);
  camera.Azimuth(static_cast<vtkm::Float32>(45.0));
  camera.Elevation(static_cast<vtkm::Float32>(45.0));
}

template <>
inline void
SetCamera<vtkm::rendering::View2D>(vtkm::rendering::Camera &camera,
                                   const vtkm::Bounds &coordBounds)
{
  camera = vtkm::rendering::Camera(vtkm::rendering::Camera::MODE_2D);
  camera.ResetToBounds(coordBounds);
  camera.SetClippingRange(1.f, 100.f);
  camera.SetViewport(-0.7f, +0.7f, -0.7f, +0.7f);
}

template <>
inline void
SetCamera<vtkm::rendering::View1D>(vtkm::rendering::Camera &camera,
                                   const vtkm::Bounds &coordBounds,                                   
                                   const vtkm::cont::Field &field)
{
  vtkm::Bounds bounds;
  bounds.X = coordBounds.X;
  field.GetRange(&bounds.Y, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    
  camera = vtkm::rendering::Camera(vtkm::rendering::Camera::MODE_1D);
  camera.ResetToBounds(bounds);
  camera.SetClippingRange(1.f, 100.f);
  camera.SetViewport(-0.7f, +0.7f, -0.7f, +0.7f);
}    

template <typename MapperType,typename CanvasType, typename ViewType>
void
Render(ViewType &view,
       const std::string &outputFile)
{
    view.Initialize();
    view.Paint();
    view.SaveAs(outputFile);
}

template <typename MapperType,typename CanvasType, typename ViewType>
void
Render(const vtkm::cont::DataSet &ds,
       const std::string &fieldNm,
       const vtkm::rendering::ColorTable &colorTable,
       const std::string &outputFile)
{
    MapperType mapper;
    CanvasType canvas(512,512);
    vtkm::rendering::Scene scene;

    scene.AddActor(vtkm::rendering::Actor(ds.GetCellSet(),
                                          ds.GetCoordinateSystem(),
                                          ds.GetField(fieldNm),
                                          colorTable));
    vtkm::rendering::Camera camera;
    SetCamera<ViewType>(camera,
                        ds.GetCoordinateSystem().GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG()));
    ViewType view(scene, mapper, canvas, camera,
                  vtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));

    Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

template <typename MapperType,typename CanvasType, typename ViewType>
void
Render(const vtkm::cont::DataSet &ds,
       const std::string &fieldNm,
       const std::string &outputFile)
{
    MapperType mapper;
    CanvasType canvas(512,512);
    vtkm::rendering::Scene scene;

    //DRP Actor? no field? no colortable (or a constant colortable) ??
    scene.AddActor(vtkm::rendering::Actor(ds.GetCellSet(),
                                          ds.GetCoordinateSystem(),
                                          ds.GetField(fieldNm)));
    vtkm::rendering::Camera camera;
    SetCamera<ViewType>(camera,
                        ds.GetCoordinateSystem().GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG()),
                        ds.GetField(fieldNm));
    ViewType view(scene, mapper, canvas, camera,
                  vtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
    Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

}}} // namespace vtkm::rendering::testing

#endif //vtk_m_rendering_testing_RenderTest_h
