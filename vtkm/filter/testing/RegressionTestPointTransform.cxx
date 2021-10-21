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

#include <vtkm/filter/PointTransform.h>
#include <vtkm/filter/VectorMagnitude.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestPointTransform()
{
  std::cout << "Generate Image for PointTransform filter with Translation" << std::endl;

  vtkm::cont::ColorTable colorTable("inferno");
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  auto pathname =
    vtkm::cont::testing::Testing::DataPath("unstructured/PointTransformTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();

  vtkm::filter::PointTransform pointTransform;
  pointTransform.SetOutputFieldName("translation");
  pointTransform.SetTranslation(vtkm::Vec3f(1, 1, 1));

  auto result = pointTransform.Execute(dataSet);

  // Need to take the magnitude of the "translation" field.
  // ColorMap only works with scalar fields (1 component)
  vtkm::filter::VectorMagnitude vectorMagnitude;
  vectorMagnitude.SetActiveField("translation");
  vectorMagnitude.SetOutputFieldName("pointvar");
  result = vectorMagnitude.Execute(result);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result, "pointvar", colorTable, "filter/point-transform.png", false);
}
} // namespace

int RegressionTestPointTransform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
