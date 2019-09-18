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
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasEGL.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View1D.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = vtkm::rendering::MapperGL;
  using C = vtkm::rendering::CanvasEGL;
  using V3 = vtkm::rendering::View3D;
  using V2 = vtkm::rendering::View2D;
  using V1 = vtkm::rendering::View1D;

  vtkm::cont::DataSetFieldAdd dsf;
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
    maker.Make1DUniformDataSet0(), "pointvar", vtkm::rendering::Color::white, "uniform1D.pnm");
  vtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DExplicitDataSet0(), "pointvar", vtkm::rendering::Color::white, "expl1D.pnm");

  vtkm::cont::DataSet ds = maker.Make1DUniformDataSet0();
  vtkm::Int32 nVerts = ds.GetField(0).GetNumberOfValues();
  vtkm::Float32 vars[nVerts];
  vtkm::Float32 smallVal = 1.000;
  for (vtkm::Int32 i = 0; i <= nVerts; i++)
  {
    vars[i] = smallVal;
    smallVal += .01;
  }
  dsf.AddPointField(ds, "smallScaledYAxis", vars, nVerts);
  vtkm::rendering::testing::Render<M, C, V1>(
    ds, "smallScaledYAxis", vtkm::rendering::Color::white, "uniform1DSmallScaledYAxis.pnm");

  // Test to demonstrate that straight horizontal lines can be drawn
  ds = maker.Make1DUniformDataSet0();
  nVerts = ds.GetField(0).GetNumberOfValues();
  vtkm::Float32 largeVal = 1e-16;
  for (vtkm::Int32 i = 0; i <= nVerts; i++)
  {
    vars[i] = largeVal;
  }
  dsf.AddPointField(ds, "straightLine", vars, nVerts);
  vtkm::rendering::testing::Render<M, C, V1>(
    ds, "straightLine", vtkm::rendering::Color::white, "uniform1DStraightLine.pnm");


  ds = maker.Make1DUniformDataSet0();
  nVerts = ds.GetField(0).GetNumberOfValues();
  largeVal = 1;
  for (vtkm::Int32 i = 0; i <= nVerts; i++)
  {
    vars[i] = largeVal;
    if (i < 2)
    {
      largeVal *= 100;
    }
    else
    {
      largeVal /= 2.25;
    }
  }
  dsf.AddPointField(ds, "logScaledYAxis", vars, nVerts);
  vtkm::rendering::testing::Render<M, C, V1>(
    ds, "logScaledYAxis", vtkm::rendering::Color::white, "uniform1DLogScaledYAxis.pnm", true);
}
} //namespace

int UnitTestMapperEGL(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
