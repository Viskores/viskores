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
#include <vtkm/rendering/MapperQuad.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::Quad;
  options.AllowAnyDevice = false;
  options.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;

  vtkm::rendering::testing::RenderTest(
    maker.Make3DRegularDataSet0(), "pointvar", "rendering/quad/regular3D.png", options);
  vtkm::rendering::testing::RenderTest(
    maker.Make3DRectilinearDataSet0(), "pointvar", "rendering/quad/rectilinear3D.png", options);
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet4(), "pointvar", "rendering/quad/explicit3D.png", options);

  //hexahedron, wedge, pyramid, tetrahedron
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet5(), "cellvar", "rendering/quad/mixed3D.png", options);

  options.ViewDimension = 2;
  vtkm::rendering::testing::RenderTest(
    maker.Make2DUniformDataSet1(), "pointvar", "rendering/quad/uniform2D.png", options);
}

} //namespace

int UnitTestMapperQuads(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
