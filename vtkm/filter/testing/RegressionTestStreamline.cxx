//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/filter/Streamline.h>
#include <vtkm/filter/Tube.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestStreamline()
{
  std::cout << "Generate Image for Streamline filter" << std::endl;

  vtkm::cont::ColorTable colorTable("inferno");
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  auto pathname = vtkm::cont::testing::Testing::DataPath("uniform/StreamlineTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray =
    vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                   vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                   vtkm::Particle(vtkm::Vec3f(.2f, 3.0f, .2f), 2) });

  vtkm::filter::Streamline streamline;
  streamline.SetStepSize(0.1f);
  streamline.SetNumberOfSteps(20);
  streamline.SetSeeds(seedArray);
  streamline.SetActiveField("vector");
  auto result = streamline.Execute(dataSet);

  // Some sort of color map is needed when rendering the coordinates of a dataset
  // so create a zeroed array for the coordinates.
  std::vector<vtkm::FloatDefault> colorMap(static_cast<std::vector<vtkm::FloatDefault>::size_type>(
    result.GetCoordinateSystem().GetNumberOfPoints()));
  for (std::vector<vtkm::FloatDefault>::size_type i = 0; i < colorMap.size(); i++)
  {
    colorMap[i] = static_cast<vtkm::FloatDefault>(i);
  }
  result.AddPointField("pointvar", colorMap);

  // The streamline by itself doesn't generate renderable geometry, so surround the
  // streamlines in tubes.
  vtkm::filter::Tube tube;
  tube.SetCapping(true);
  tube.SetNumberOfSides(3);
  tube.SetRadius(static_cast<vtkm::FloatDefault>(0.2));
  result = tube.Execute(result);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result, "pointvar", colorTable, "filter/streamline.png", false);
}
} // namespace

int RegressionTestStreamline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamline, argc, argv);
}
