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
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/filter/field_transform/PointTransform.h>
#include <vtkm/filter/vector_analysis/VectorMagnitude.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestPointTransform()
{
  std::cout << "Generate Image for PointTransform filter with Translation" << std::endl;

  auto pathname =
    vtkm::cont::testing::Testing::DataPath("unstructured/PointTransformTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();

  vtkm::filter::field_transform::PointTransform pointTransform;
  pointTransform.SetOutputFieldName("translation");
  pointTransform.SetTranslation(vtkm::Vec3f(1, 1, 1));

  auto result = pointTransform.Execute(dataSet);

  // Need to take the magnitude of the "translation" field.
  // ColorMap only works with scalar fields (1 component)
  vtkm::filter::vector_analysis::VectorMagnitude vectorMagnitude;
  vectorMagnitude.SetActiveField("translation");
  vectorMagnitude.SetOutputFieldName("pointvar");
  result = vectorMagnitude.Execute(result);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(
    result, "pointvar", "filter/point-transform.png", testOptions);
}
} // namespace

int RenderTestPointTransform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
