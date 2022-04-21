//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/contour/Slice.h>

#include <vtkm/ImplicitFunction.h>
#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>
#include <vtkm/source/Wavelet.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{

void TestSliceStructuredPointsPlane()
{
  std::cout << "Generate Image for Slice by plane on structured points" << std::endl;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();

  vtkm::Plane plane(vtkm::Plane::Vector{ 1, 1, 1 });
  vtkm::filter::contour::Slice slice;
  slice.SetImplicitFunction(plane);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  vtkm::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-structured-points-plane.png", testOptions);
}

void TestSliceStructuredPointsSphere()
{
  std::cout << "Generate Image for Slice by sphere on structured points" << std::endl;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();

  vtkm::Sphere sphere(8.5f);
  vtkm::filter::contour::Slice slice;
  slice.SetImplicitFunction(sphere);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  vtkm::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-structured-points-sphere.png", testOptions);
}

void TestSliceUnstructuredGridPlane()
{
  std::cout << "Generate Image for Slice by plane on unstructured grid" << std::endl;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();
  vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralize;
  ds = tetrahedralize.Execute(ds);

  vtkm::Plane plane(vtkm::Plane::Vector{ 1 });
  vtkm::filter::contour::Slice slice;
  slice.SetImplicitFunction(plane);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  vtkm::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-unstructured-grid-plane.png", testOptions);
}

void TestSliceUnstructuredGridCylinder()
{
  std::cout << "Generate Image for Slice by cylinder on unstructured grid" << std::endl;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();
  vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralize;
  ds = tetrahedralize.Execute(ds);

  vtkm::Cylinder cylinder(vtkm::Cylinder::Vector{ 0, 1, 0 }, 8.5f);
  vtkm::filter::contour::Slice slice;
  slice.SetImplicitFunction(cylinder);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  vtkm::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-unstructured-grid-cylinder.png", testOptions);
}

void TestSliceFilter()
{
  TestSliceStructuredPointsPlane();
  TestSliceStructuredPointsSphere();
  TestSliceUnstructuredGridPlane();
  TestSliceUnstructuredGridCylinder();
}

} // namespace

int RenderTestSliceFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSliceFilter, argc, argv);
}
