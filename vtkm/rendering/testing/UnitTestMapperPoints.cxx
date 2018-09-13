//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DeviceAdapter.h>
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
  using M = vtkm::rendering::MapperPoint;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::ColorTable colorTable("inferno");

  M mapper;
  std::cout << "Testing uniform delta raduis\n";
  mapper.SetRadiusDelta(4.0f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DUniformDataSet1(), "pointvar", colorTable, "points_vr_reg3D.pnm");

  // restore defaults
  mapper.SetRadiusDelta(0.5f);
  mapper.UseVariableRadius(false);

  mapper.SetRadius(0.2f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DUniformDataSet1(), "pointvar", colorTable, "points_reg3D.pnm");

  mapper.UseCells();
  mapper.SetRadius(1.f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DExplicitDataSet7(), "cellvar", colorTable, "spheres.pnm");
}

} //namespace

int UnitTestMapperPoints(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RenderTests);
}
