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

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
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
  typedef vtkm::rendering::MapperVolume M;
  typedef vtkm::rendering::MapperRayTracer R;
  typedef vtkm::rendering::CanvasRayTracer C;
  typedef vtkm::rendering::View3D V3;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::rendering::ColorTable colorTable("thermal");


  vtkm::rendering::ColorTable colorTable2("cool2warm");
  colorTable2.AddAlphaControlPoint(0.0, .02);
  colorTable2.AddAlphaControlPoint(1.0, .02);

  vtkm::rendering::testing::MultiMapperRender<R, M, C, V3>(maker.Make3DExplicitDataSet4(),
                                                           maker.Make3DRectilinearDataSet0(),
                                                           "pointvar",
                                                           colorTable,
                                                           colorTable2,
                                                           "multi.pnm");
}

} //namespace

int UnitTestMultiMapper(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RenderTests);
}
