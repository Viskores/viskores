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
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/testing/Testing.h>

namespace
{


void RenderTests()
{
  using M = vtkm::rendering::MapperVolume;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::cont::ColorTable colorTable("inferno");
  colorTable.AddPointAlpha(0.0, .01f);
  colorTable.AddPointAlpha(1.0, .01f);

  vtkm::cont::DataSet rectDS, unsDS;
  std::string rectfname = vtkm::cont::testing::Testing::DataPath("rectilinear/noise.vtk");
  vtkm::io::VTKDataSetReader rectReader(rectfname);

  try
  {
    rectDS = rectReader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += rectfname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  vtkm::rendering::testing::Render<M, C, V3>(rectDS, "hardyglobal", colorTable, "rect3D.pnm");
}

} //namespace

int UnitTestMapperVolume(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
