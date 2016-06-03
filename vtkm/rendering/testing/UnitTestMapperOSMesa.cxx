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
#include <vtkm/Bounds.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasOSMesa.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/WorldAnnotatorGL.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>

namespace {

void Set3DView(vtkm::rendering::Camera &camera,
               const vtkm::cont::CoordinateSystem &coords)
{
    vtkm::Bounds coordsBounds = coords.GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view
    vtkm::Vec<vtkm::Float32,3> totalExtent;
    totalExtent[0] = vtkm::Float32(coordsBounds.X.Max - coordsBounds.X.Min);
    totalExtent[1] = vtkm::Float32(coordsBounds.Y.Max - coordsBounds.Y.Min);
    totalExtent[2] = vtkm::Float32(coordsBounds.Z.Max - coordsBounds.Z.Min);
    vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
    vtkm::Normalize(totalExtent);

    camera = vtkm::rendering::Camera(vtkm::rendering::Camera::VIEW_3D);
    camera.Camera3d.Position = totalExtent * (mag * 2.f);
    camera.Camera3d.Up = vtkm::Vec<vtkm::Float32,3>(0.f, 1.f, 0.f);
    camera.Camera3d.LookAt = totalExtent * (mag * .5f);
    camera.Camera3d.FieldOfView = 60.f;
    camera.NearPlane = 1.f;
    camera.FarPlane = 100.f;
    /*
    std::cout<<"Camera3d: pos: "<<camera.camera3d.pos<<std::endl;
    std::cout<<"       lookAt: "<<camera.camera3d.lookAt<<std::endl;
    std::cout<<"           up: "<<camera.camera3d.up<<std::endl;
    std::cout<<" near/far/fov: "<<camera.nearPlane<<"/"<<camera.farPlane<<" "<<camera.camera3d.fieldOfView<<std::endl;
    std::cout<<"          w/h: "<<camera.width<<"/"<<camera.height<<std::endl;
    */
}

void Set2DView(vtkm::rendering::Camera &camera,
               const vtkm::cont::CoordinateSystem &coords,
               vtkm::Int32 w, vtkm::Int32 h)
{
    vtkm::Bounds coordsBounds = coords.GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view

    camera = vtkm::rendering::Camera(vtkm::rendering::Camera::VIEW_2D);
    camera.Camera2d.Left = static_cast<vtkm::Float32>(coordsBounds.X.Min);
    camera.Camera2d.Right = static_cast<vtkm::Float32>(coordsBounds.X.Max);
    camera.Camera2d.Bottom = static_cast<vtkm::Float32>(coordsBounds.Y.Min);
    camera.Camera2d.Top = static_cast<vtkm::Float32>(coordsBounds.Y.Max);
    camera.NearPlane = 1.f;
    camera.FarPlane = 100.f;

    // Give it some space for other annotations like a color bar
    camera.ViewportLeft = -.7f;
    camera.ViewportRight = +.7f;
    camera.ViewportBottom = -.7f;
    camera.ViewportTop = +.7f;

    /*
    std::cout<<"Camera2d:  l/r: "<<camera.camera2d.left<<" "<<camera.camera2d.right<<std::endl;
    std::cout<<"Camera2d:  b/t: "<<camera.camera2d.bottom<<" "<<camera.camera2d.top<<std::endl;
    std::cout<<"    near/far: "<<camera.nearPlane<<"/"<<camera.farPlane<<std::endl;
    std::cout<<"         w/h: "<<camera.width<<"/"<<camera.height<<std::endl;
    */
}

void Render3D(const vtkm::cont::DataSet &ds,
              const std::string &fieldNm,
              const std::string &ctName,
              const std::string &outputFile)
{
    const vtkm::Int32 W = 512, H = 512;
    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> mapper;

    vtkm::rendering::Camera camera;
    Set3DView(camera, coords);

    vtkm::rendering::Scene scene;
    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::CanvasOSMesa canvas(W,H,bg);

    scene.Actors.push_back(vtkm::rendering::Actor(ds.GetCellSet(),
                                                  ds.GetCoordinateSystem(),
                                                  ds.GetField(fieldNm),
                                                  vtkm::rendering::ColorTable(ctName)));

    //TODO: W/H in view.  bg in view (view sets canvas/renderer).
    vtkm::rendering::View3D<vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                            vtkm::rendering::CanvasOSMesa,
                            vtkm::rendering::WorldAnnotatorGL>
        w(scene, mapper, canvas, camera, bg);

    w.Initialize();
    w.Paint();
    w.SaveAs(outputFile);
}

void Render2D(const vtkm::cont::DataSet &ds,
              const std::string &fieldNm,
              const std::string &ctName,
              const std::string &outputFile)
{
    const vtkm::Int32 W = 512, H = 512;
    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> mapper;

    vtkm::rendering::Camera camera;
    Set2DView(camera, coords, W, H);

    vtkm::rendering::Scene scene;
    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::CanvasOSMesa canvas(W,H,bg);

    scene.AddActor(vtkm::rendering::Actor(ds.GetCellSet(),
                                          ds.GetCoordinateSystem(),
                                          ds.GetField(fieldNm),
                                          vtkm::rendering::ColorTable(ctName)));
    vtkm::rendering::View2D<vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                            vtkm::rendering::CanvasOSMesa,
                            vtkm::rendering::WorldAnnotatorGL>
        w(scene, mapper, canvas, camera, bg);

    w.Initialize();
    w.Paint();
    w.SaveAs(outputFile);

}

void RenderTests()
{
    vtkm::cont::testing::MakeTestDataSet maker;

    //3D tests.
    Render3D(maker.Make3DRegularDataSet0(),
             "pointvar", "thermal", "reg3D.pnm");
    Render3D(maker.Make3DRectilinearDataSet0(),
             "pointvar", "thermal", "rect3D.pnm");
    Render3D(maker.Make3DExplicitDataSet4(),
             "pointvar", "thermal", "expl3D.pnm");

    //2D tests.
    Render2D(maker.Make2DRectilinearDataSet0(),
             "pointvar", "thermal", "rect2D.pnm");
}
} //namespace

int UnitTestMapperOSMesa(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(RenderTests);
}
