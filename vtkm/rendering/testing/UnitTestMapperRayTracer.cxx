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
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>

namespace {

void Set3DView(vtkm::rendering::Camera &camera,
               const vtkm::cont::CoordinateSystem &coords)
{
    vtkm::Bounds coordsBounds =
        coords.GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view
    camera = vtkm::rendering::Camera();
    camera.ResetToBounds(coordsBounds);
    camera.Azimuth(45.0f);
    camera.Elevation(45.0f);

    std::cout << "Camera3d: pos: " << camera.GetPosition() << std::endl;
    std::cout << "       lookAt: " << camera.GetLookAt() << std::endl;
    std::cout << "           up: " << camera.GetViewUp() << std::endl;
    std::cout << "     near/far: " << camera.GetClippingRange() << std::endl;
    std::cout << "  fieldOfView: " << camera.GetFieldOfView() << std::endl;
}

void Render(const vtkm::cont::DataSet &ds,
            const std::string &fieldNm,
            const std::string &ctName,
            const std::string &outputFile)
{
    const vtkm::Int32 W = 512, H = 512;
    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    vtkm::rendering::MapperRayTracer<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> mapper;

    vtkm::rendering::Camera camera;
    Set3DView(camera, coords);

    vtkm::rendering::Scene scene;
    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::CanvasRayTracer canvas(W,H);
    scene.AddActor(vtkm::rendering::Actor(ds.GetCellSet(),
                                          ds.GetCoordinateSystem(),
                                          ds.GetField(fieldNm),
                                          vtkm::rendering::ColorTable(ctName)));

    vtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);

    view.Initialize();
    view.Paint();
    view.SaveAs(outputFile);
}

void RenderTests()
{
    vtkm::cont::testing::MakeTestDataSet maker;

    //3D tests.
    Render(maker.Make3DRegularDataSet0(),
             "pointvar", "thermal", "reg3D.pnm");
    Render(maker.Make3DRectilinearDataSet0(),
             "pointvar", "thermal", "rect3D.pnm");
    Render(maker.Make3DExplicitDataSet4(),
             "pointvar", "thermal", "expl3D.pnm");
}

} //namespace
int UnitTestMapperRayTracer(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(RenderTests);
}
