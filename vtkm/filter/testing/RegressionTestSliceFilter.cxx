//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/Slice.h>

#include <vtkm/ImplicitFunction.h>
#include <vtkm/filter/Tetrahedralize.h>
#include <vtkm/source/Wavelet.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{

void TestSliceStructuredPointsPlane()
{
  std::cout << "Generate Image for Slice by plane on structured points" << std::endl;

  vtkm::cont::ColorTable colorTable;
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();

  vtkm::Plane plane(vtkm::Plane::Vector{ 1, 1, 1 });
  vtkm::filter::Slice slice;
  slice.SetImplicitFunction(plane);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result,
    "scalars",
    colorTable,
    "filter/slice-structured-points-plane.png",
    false,
    static_cast<vtkm::FloatDefault>(0.08));
}

void TestSliceStructuredPointsSphere()
{
  std::cout << "Generate Image for Slice by sphere on structured points" << std::endl;

  vtkm::cont::ColorTable colorTable;
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();

  vtkm::Sphere sphere(8.5f);
  vtkm::filter::Slice slice;
  slice.SetImplicitFunction(sphere);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result,
    "scalars",
    colorTable,
    "filter/slice-structured-points-sphere.png",
    false,
    static_cast<vtkm::FloatDefault>(0.08));
}

void TestSliceUnstructuredGridPlane()
{
  std::cout << "Generate Image for Slice by plane on unstructured grid" << std::endl;

  vtkm::cont::ColorTable colorTable;
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();
  vtkm::filter::Tetrahedralize tetrahedralize;
  ds = tetrahedralize.Execute(ds);

  vtkm::Plane plane(vtkm::Plane::Vector{ 1 });
  vtkm::filter::Slice slice;
  slice.SetImplicitFunction(plane);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result,
    "scalars",
    colorTable,
    "filter/slice-unstructured-grid-plane.png",
    false,
    static_cast<vtkm::FloatDefault>(0.08));
}

void TestSliceUnstructuredGridCylinder()
{
  std::cout << "Generate Image for Slice by cylinder on unstructured grid" << std::endl;

  vtkm::cont::ColorTable colorTable;
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::source::Wavelet wavelet(vtkm::Id3(-8), vtkm::Id3(8));
  auto ds = wavelet.Execute();
  vtkm::filter::Tetrahedralize tetrahedralize;
  ds = tetrahedralize.Execute(ds);

  vtkm::Cylinder cylinder(vtkm::Cylinder::Vector{ 0, 1, 0 }, 8.5f);
  vtkm::filter::Slice slice;
  slice.SetImplicitFunction(cylinder);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result,
    "scalars",
    colorTable,
    "filter/slice-unstructured-grid-cylinder.png",
    false,
    static_cast<vtkm::FloatDefault>(0.08));
}

void TestSliceFilter()
{
  TestSliceStructuredPointsPlane();
  TestSliceStructuredPointsSphere();
  TestSliceUnstructuredGridPlane();
  TestSliceUnstructuredGridCylinder();
}

} // namespace

int RegressionTestSliceFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSliceFilter, argc, argv);
}
