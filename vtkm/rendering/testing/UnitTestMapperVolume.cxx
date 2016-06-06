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
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/WorldAnnotator.h>
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
}

void Render(const vtkm::cont::DataSet &ds,
            const std::string &fieldNm,
            const std::string &ctName,
            const std::string &outputFile)
{
    const vtkm::Int32 W = 512, H = 512;
    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    vtkm::rendering::MapperVolume<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> mapper;
    vtkm::rendering::WorldAnnotator annotator;

    vtkm::rendering::Camera camera;
    Set3DView(camera, coords);

    vtkm::rendering::ColorTable colorTable(ctName);
    colorTable.AddAlphaControlPoint(0.0f, .01f);
    colorTable.AddAlphaControlPoint(1.0f, .01f);

    vtkm::rendering::Scene scene;
    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::CanvasRayTracer canvas(W,H,bg);
    scene.AddActor(vtkm::rendering::Actor(ds.GetCellSet(),
                                          ds.GetCoordinateSystem(),
                                          ds.GetField(fieldNm),
                                          colorTable));

    //TODO: W/H in view.  bg in view (view sets canvas/renderer).
    vtkm::rendering::View3D view(scene, mapper, canvas, annotator, camera, bg);

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
}

} //namespace
int UnitTestMapperVolume(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(RenderTests);
}
