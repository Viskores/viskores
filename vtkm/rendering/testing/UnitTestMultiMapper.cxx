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
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperConnectivity.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

#include <vtkm/rendering/raytracing/RayOperations.h>

namespace
{

void RenderTests()
{
  using M1 = vtkm::rendering::MapperVolume;
  using M2 = vtkm::rendering::MapperConnectivity;
  using R = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::ColorTable colorTable("inferno");


  vtkm::cont::ColorTable colorTable2("cool to warm");
  colorTable2.AddPointAlpha(0.0, .02f);
  colorTable2.AddPointAlpha(1.0, .02f);

  vtkm::rendering::testing::MultiMapperRender<R, M2, C, V3>(maker.Make3DExplicitDataSetPolygonal(),
                                                            maker.Make3DRectilinearDataSet0(),
                                                            "pointvar",
                                                            colorTable,
                                                            colorTable2,
                                                            "multi1.pnm");

  vtkm::rendering::testing::MultiMapperRender<R, M1, C, V3>(maker.Make3DExplicitDataSet4(),
                                                            maker.Make3DRectilinearDataSet0(),
                                                            "pointvar",
                                                            colorTable,
                                                            colorTable2,
                                                            "multi2.pnm");
}

} //namespace

int UnitTestMultiMapper(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
