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
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperConnectivity.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.AllowedPixelErrorRatio = 0.002f;
  testOptions.Mapper = vtkm::rendering::testing::MapperType::Connectivity;
  testOptions.AllowAnyDevice = false;
  testOptions.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;

  vtkm::rendering::testing::RenderTest(
    maker.Make3DRegularDataSet0(), "pointvar", "rendering/connectivity/regular3D.png", testOptions);
  vtkm::rendering::testing::RenderTest(maker.Make3DRectilinearDataSet0(),
                                       "pointvar",
                                       "rendering/connectivity/rectilinear3D.png",
                                       testOptions);
  vtkm::rendering::testing::RenderTest(maker.Make3DExplicitDataSetZoo(),
                                       "pointvar",
                                       "rendering/connectivity/explicit3D.png",
                                       testOptions);
}

} //namespace

int UnitTestMapperConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
