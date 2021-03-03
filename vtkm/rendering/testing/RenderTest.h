//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_testing_RenderTest_h
#define vtk_m_rendering_testing_RenderTest_h

#include <vtkm/Bounds.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/TextAnnotationScreen.h>
#include <vtkm/rendering/View1D.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/Testing.h>

#include <memory>

namespace vtkm
{
namespace rendering
{
namespace testing
{

template <typename ViewType>
inline void SetCamera(vtkm::rendering::Camera& camera,
                      const vtkm::Bounds& coordBounds,
                      const vtkm::cont::Field& field,
                      const vtkm::Float64& dataViewPadding = 0);

template <>
inline void SetCamera<vtkm::rendering::View3D>(vtkm::rendering::Camera& camera,
                                               const vtkm::Bounds& coordBounds,
                                               const vtkm::cont::Field&,
                                               const vtkm::Float64& dataViewPadding)
{
  vtkm::Bounds b = coordBounds;
  camera = vtkm::rendering::Camera();
  camera.ResetToBounds(b, dataViewPadding);
  camera.Azimuth(static_cast<vtkm::Float32>(45.0));
  camera.Elevation(static_cast<vtkm::Float32>(45.0));
}

template <>
inline void SetCamera<vtkm::rendering::View2D>(vtkm::rendering::Camera& camera,
                                               const vtkm::Bounds& coordBounds,
                                               const vtkm::cont::Field&,
                                               const vtkm::Float64& dataViewPadding)
{
  camera = vtkm::rendering::Camera(vtkm::rendering::Camera::MODE_2D);
  camera.ResetToBounds(coordBounds, dataViewPadding);
  camera.SetClippingRange(1.f, 100.f);
  camera.SetViewport(-0.7f, +0.7f, -0.7f, +0.7f);
}

template <>
inline void SetCamera<vtkm::rendering::View1D>(vtkm::rendering::Camera& camera,
                                               const vtkm::Bounds& coordBounds,
                                               const vtkm::cont::Field& field,
                                               const vtkm::Float64& dataViewPadding)
{
  vtkm::Bounds bounds;
  bounds.X = coordBounds.X;
  field.GetRange(&bounds.Y);

  camera = vtkm::rendering::Camera(vtkm::rendering::Camera::MODE_2D);
  camera.ResetToBounds(bounds, dataViewPadding);
  camera.SetClippingRange(1.f, 100.f);
  camera.SetViewport(-0.7f, +0.7f, -0.7f, +0.7f);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(ViewType& view, const std::string& outputFile)
{
  view.Paint();
  view.SaveAs(outputFile);
}

// Different methods in which to create a View when provided with different
// field names, colors, and other utilities for testing 1D, 2D, and 3D views

// Testing Methods for working with what are normally 3D datasets
// --------------------------------------------------------------
template <typename MapperType, typename CanvasType, typename ViewType>
std::shared_ptr<ViewType> GetViewPtr(const vtkm::cont::DataSet& ds,
                                     const std::string& fieldNm,
                                     const CanvasType& canvas,
                                     const MapperType& mapper,
                                     vtkm::rendering::Scene& scene,
                                     const vtkm::cont::ColorTable& colorTable,
                                     const vtkm::Float64& dataViewPadding = 0)
{
  scene.AddActor(vtkm::rendering::Actor(
    ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fieldNm), colorTable));
  vtkm::rendering::Camera camera;
  SetCamera<ViewType>(
    camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fieldNm), dataViewPadding);
  vtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  vtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  auto view = std::make_shared<ViewType>(scene, mapper, canvas, camera, background, foreground);

  // Print the title
  std::unique_ptr<vtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new vtkm::rendering::TextAnnotationScreen(
      "Test Plot", vtkm::rendering::Color(1, 1, 1, 1), .075f, vtkm::Vec2f_32(-.11f, .92f), 0.f));
  view->AddTextAnnotation(std::move(titleAnnotation));
  return view;
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(const vtkm::cont::DataSet& ds,
            const std::string& fieldNm,
            const vtkm::cont::ColorTable& colorTable,
            const std::string& outputFile,
            const vtkm::Float64& dataViewPadding = 0)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fieldNm, canvas, mapper, scene, colorTable, dataViewPadding);
  Render<MapperType, CanvasType, ViewType>(*view, outputFile);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void RenderAndRegressionTest(const vtkm::cont::DataSet& ds,
                             const std::string& fieldNm,
                             const vtkm::cont::ColorTable& colorTable,
                             const std::string& outputFile,
                             const bool& enableAnnotations = true,
                             const vtkm::Float64& dataViewPadding = 0)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fieldNm, canvas, mapper, scene, colorTable, dataViewPadding);
  view->SetRenderAnnotationsEnabled(enableAnnotations);
  VTKM_TEST_ASSERT(test_equal_images(view, outputFile));
}
// --------------------------------------------------------------

// Testing Methods for working with what are normally 2D datasets
// --------------------------------------------------------------
template <typename MapperType, typename CanvasType, typename ViewType>
std::shared_ptr<ViewType> GetViewPtr(const vtkm::cont::DataSet& ds,
                                     const std::vector<std::string>& fields,
                                     const std::vector<vtkm::rendering::Color>& colors,
                                     const CanvasType& canvas,
                                     const MapperType& mapper,
                                     vtkm::rendering::Scene& scene,
                                     const vtkm::Float64& dataViewPadding = 0)
{
  size_t numFields = fields.size();
  for (size_t i = 0; i < numFields; ++i)
  {
    scene.AddActor(vtkm::rendering::Actor(
      ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fields[i]), colors[i]));
  }
  vtkm::rendering::Camera camera;
  SetCamera<ViewType>(
    camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fields[0]), dataViewPadding);

  vtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  vtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  auto view = std::make_shared<ViewType>(scene, mapper, canvas, camera, background, foreground);

  // Print the title
  std::unique_ptr<vtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new vtkm::rendering::TextAnnotationScreen(
      "Test Plot", vtkm::rendering::Color(1, 1, 1, 1), .075f, vtkm::Vec2f_32(-.11f, .92f), 0.f));
  view->AddTextAnnotation(std::move(titleAnnotation));
  return view;
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(const vtkm::cont::DataSet& ds,
            const std::vector<std::string>& fields,
            const std::vector<vtkm::rendering::Color>& colors,
            const std::string& outputFile,
            const vtkm::Float64& dataViewPadding = 0)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  canvas.SetBackgroundColor(vtkm::rendering::Color::white);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fields, colors, canvas, mapper, scene, dataViewPadding);
  Render<MapperType, CanvasType, ViewType>(*view, outputFile);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void RenderAndRegressionTest(const vtkm::cont::DataSet& ds,
                             const std::vector<std::string>& fields,
                             const std::vector<vtkm::rendering::Color>& colors,
                             const std::string& outputFile,
                             const bool& enableAnnotations = true,
                             const vtkm::Float64& dataViewPadding = 0)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  canvas.SetBackgroundColor(vtkm::rendering::Color::white);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fields, colors, canvas, mapper, scene, dataViewPadding);
  view->SetRenderAnnotationsEnabled(enableAnnotations);
  VTKM_TEST_ASSERT(test_equal_images(view, outputFile));
}
// --------------------------------------------------------------

// Testing Methods for working with what are normally 1D datasets
// --------------------------------------------------------------
template <typename MapperType, typename CanvasType, typename ViewType>
std::shared_ptr<ViewType> GetViewPtr(const vtkm::cont::DataSet& ds,
                                     const std::string& fieldNm,
                                     const CanvasType& canvas,
                                     const MapperType& mapper,
                                     vtkm::rendering::Scene& scene,
                                     const vtkm::rendering::Color& color,
                                     const bool logY = false,
                                     const vtkm::Float64& dataViewPadding = 0)
{
  //DRP Actor? no field? no colortable (or a constant colortable) ??
  scene.AddActor(
    vtkm::rendering::Actor(ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fieldNm), color));
  vtkm::rendering::Camera camera;
  SetCamera<ViewType>(
    camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fieldNm), dataViewPadding);

  vtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  vtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  auto view = std::make_shared<ViewType>(scene, mapper, canvas, camera, background, foreground);

  // Print the title
  std::unique_ptr<vtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new vtkm::rendering::TextAnnotationScreen(
      "1D Test Plot", foreground, .1f, vtkm::Vec2f_32(-.27f, .87f), 0.f));
  view->AddTextAnnotation(std::move(titleAnnotation));
  view->SetLogY(logY);
  return view;
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(const vtkm::cont::DataSet& ds,
            const std::string& fieldNm,
            const vtkm::rendering::Color& color,
            const std::string& outputFile,
            const bool logY = false,
            const vtkm::Float64& dataViewPadding = 0)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fieldNm, canvas, mapper, scene, color, logY, dataViewPadding);
  Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void RenderAndRegressionTest(const vtkm::cont::DataSet& ds,
                             const std::string& fieldNm,
                             const vtkm::rendering::Color& color,
                             const std::string& outputFile,
                             const bool logY = false,
                             const bool& enableAnnotations = true,
                             const vtkm::Float64& dataViewPadding = 0)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fieldNm, canvas, mapper, scene, color, logY, dataViewPadding);
  view->SetRenderAnnotationsEnabled(enableAnnotations);
  VTKM_TEST_ASSERT(test_equal_images(view, outputFile));
}
// --------------------------------------------------------------

// A render test that allows for testing different mapper params
template <typename MapperType, typename CanvasType, typename ViewType>
void Render(MapperType& mapper,
            const vtkm::cont::DataSet& ds,
            const std::string& fieldNm,
            const vtkm::cont::ColorTable& colorTable,
            const std::string& outputFile,
            const vtkm::Float64& dataViewPadding = 0)
{
  CanvasType canvas(512, 512);
  vtkm::rendering::Scene scene;

  auto view = GetViewPtr<MapperType, CanvasType, ViewType>(
    ds, fieldNm, canvas, mapper, scene, colorTable, dataViewPadding);
  Render<MapperType, CanvasType, ViewType>(*view, outputFile);
}

template <typename MapperType1, typename MapperType2, typename CanvasType, typename ViewType>
void MultiMapperRender(const vtkm::cont::DataSet& ds1,
                       const vtkm::cont::DataSet& ds2,
                       const std::string& fieldNm,
                       const vtkm::cont::ColorTable& colorTable1,
                       const vtkm::cont::ColorTable& colorTable2,
                       const std::string& outputFile,
                       const vtkm::Float64& dataViewPadding = 0)
{
  MapperType1 mapper1;
  MapperType2 mapper2;

  CanvasType canvas(512, 512);
  canvas.SetBackgroundColor(vtkm::rendering::Color(0.8f, 0.8f, 0.8f, 1.0f));
  canvas.Clear();

  vtkm::Bounds totalBounds =
    ds1.GetCoordinateSystem().GetBounds() + ds2.GetCoordinateSystem().GetBounds();
  vtkm::rendering::Camera camera;
  SetCamera<ViewType>(camera, totalBounds, ds1.GetField(fieldNm), dataViewPadding);

  mapper1.SetCanvas(&canvas);
  mapper1.SetActiveColorTable(colorTable1);
  mapper1.SetCompositeBackground(false);

  mapper2.SetCanvas(&canvas);
  mapper2.SetActiveColorTable(colorTable2);

  const vtkm::cont::Field field1 = ds1.GetField(fieldNm);
  vtkm::Range range1;
  field1.GetRange(&range1);

  const vtkm::cont::Field field2 = ds2.GetField(fieldNm);
  vtkm::Range range2;
  field2.GetRange(&range2);

  mapper1.RenderCells(
    ds1.GetCellSet(), ds1.GetCoordinateSystem(), field1, colorTable1, camera, range1);

  mapper2.RenderCells(
    ds2.GetCellSet(), ds2.GetCoordinateSystem(), field2, colorTable2, camera, range2);

  canvas.SaveAs(outputFile);
}

} // namespace vtkm::rendering::testing
} // namespace vtkm::rendering
} // namespace vtkm

#endif //vtk_m_rendering_testing_RenderTest_h
