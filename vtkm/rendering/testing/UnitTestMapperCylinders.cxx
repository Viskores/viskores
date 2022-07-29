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
#include <vtkm/rendering/MapperCylinder.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::Cylinder;
  options.AllowAnyDevice = false;
  options.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;

  vtkm::rendering::testing::RenderTest(
    maker.Make3DRegularDataSet0(), "pointvar", "rendering/cylinder/regular3D.png", options);
  vtkm::rendering::testing::RenderTest(
    maker.Make3DRectilinearDataSet0(), "pointvar", "rendering/cylinder/rectilinear3D.png", options);
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet4(), "pointvar", "rendering/cylinder/explicit-hex.png", options);

  options.ViewDimension = 2;
  vtkm::rendering::testing::RenderTest(
    maker.Make2DUniformDataSet1(), "pointvar", "rendering/cylinder/uniform2D.png", options);

  options.ViewDimension = 3;
  options.CameraAzimuth = 0;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet8(), "cellvar", "rendering/cylinder/explicit-lines.png", options);

  //hexahedron, wedge, pyramid, tetrahedron
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet5(), "cellvar", "rendering/cylinder/explicit-zoo.png", options);

  options.CylinderRadius = 0.1f;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet8(), "cellvar", "rendering/cylinder/static-radius.png", options);

  options.CylinderUseVariableRadius = true;
  options.CylinderRadius = 2.0f;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet8(), "cellvar", "rendering/cylinder/variable-radius.png", options);
}

} //namespace

int UnitTestMapperCylinders(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
