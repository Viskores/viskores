//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::RayTracer;
  options.AllowAnyDevice = false;
  options.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;

  vtkm::rendering::testing::RenderTest(
    maker.Make3DRegularDataSet0(), "pointvar", "rendering/raytracer/regular3D.png", options);
  vtkm::rendering::testing::RenderTest(maker.Make3DRectilinearDataSet0(),
                                       "pointvar",
                                       "rendering/raytracer/rectilinear3D.png",
                                       options);
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet4(), "pointvar", "rendering/raytracer/explicit3D.png", options);

  // The result is blank. I don't think MapperRayTracer is actually supposed to render anything
  // for 0D (vertex) cells, but it shouldn't crash if it receives them
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet7(), "cellvar", "rendering/raytracer/vertex-cells.png", options);

  options.ViewDimension = 2;
  vtkm::rendering::testing::RenderTest(
    maker.Make2DUniformDataSet1(), "pointvar", "rendering/raytracer/uniform2D.png", options);
}

} //namespace

int UnitTestMapperRayTracer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
