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
#include <vtkm/Particle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/filter/geometry_refinement/Tube.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestStreamline()
{
  std::cout << "Generate Image for Streamline filter" << std::endl;

  auto pathname = vtkm::cont::testing::Testing::DataPath("uniform/StreamlineTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray =
    vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                   vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                   vtkm::Particle(vtkm::Vec3f(.2f, 3.0f, .2f), 2) });

  vtkm::filter::flow::Streamline streamline;
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
  vtkm::filter::geometry_refinement::Tube tube;
  tube.SetCapping(true);
  tube.SetNumberOfSides(3);
  tube.SetRadius(static_cast<vtkm::FloatDefault>(0.2));
  result = tube.Execute(result);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(result, "pointvar", "filter/streamline.png", testOptions);
}
} // namespace

int RenderTestStreamline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamline, argc, argv);
}
