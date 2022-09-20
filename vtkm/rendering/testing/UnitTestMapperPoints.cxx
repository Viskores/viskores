//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperPoint.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::Point;
  options.AllowAnyDevice = false;
  options.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;

  vtkm::rendering::testing::RenderTest(
    maker.Make3DUniformDataSet1(), "pointvar", "rendering/point/regular3D.png", options);

  options.UseVariableRadius = true;
  options.RadiusDelta = 4.0f;
  options.Radius = 0.25f;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DUniformDataSet1(), "pointvar", "rendering/point/variable_regular3D.png", options);

  // restore defaults
  options.RadiusDelta = 0.5f;
  options.UseVariableRadius = false;

  options.RenderCells = true;
  options.Radius = 1.f;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet7(), "cellvar", "rendering/point/cells.png", options);
}

} //namespace

int UnitTestMapperPoints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
