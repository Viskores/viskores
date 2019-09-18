//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/Bounds.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasOSMesa.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = vtkm::rendering::MapperGL;
  using C = vtkm::rendering::CanvasOSMesa;
  using V3 = vtkm::rendering::View3D;
  using V2 = vtkm::rendering::View2D;
  using V1 = vtkm::rendering::View1D;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::ColorTable colorTable("inferno");

  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
  vtkm::rendering::testing::Render<M, C, V2>(
    maker.Make2DRectilinearDataSet0(), "pointvar", colorTable, "rect2D.pnm");
  vtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DUniformDataSet0(), "pointvar", vtkm::rendering::Color(1, 1, 1, 1), "uniform1D.pnm");
  vtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DExplicitDataSet0(), "pointvar", vtkm::rendering::Color(1, 1, 1, 1), "expl1D.pnm");
}

} //namespace

int UnitTestMapperOSMesa(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
