//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/testing/RenderTest.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperPoint.h>
#include <vtkm/rendering/MapperQuad.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/TextAnnotationScreen.h>
#include <vtkm/rendering/View1D.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/cont/RuntimeDeviceTracker.h>

namespace
{

using DataSetFieldVector = std::vector<std::pair<vtkm::cont::DataSet, std::string>>;

void SetupView(vtkm::rendering::View3D& view,
               const vtkm::Bounds& bounds,
               const vtkm::Range&,
               const vtkm::rendering::testing::RenderTestOptions& options)
{
  vtkm::rendering::Camera camera;
  camera.ResetToBounds(bounds, options.DataViewPadding);
  camera.Azimuth(options.CameraAzimuth);
  camera.Elevation(options.CameraElevation);
  view.SetCamera(camera);
}

void SetupView(vtkm::rendering::View2D& view,
               const vtkm::Bounds& bounds,
               const vtkm::Range&,
               const vtkm::rendering::testing::RenderTestOptions& options)
{
  vtkm::rendering::Camera camera;
  camera.ResetToBounds(bounds, options.DataViewPadding);
  camera.SetClippingRange(options.ClippingRange);
  camera.SetViewport(options.Viewport);
  view.SetCamera(camera);
}

void SetupView(vtkm::rendering::View1D& view,
               const vtkm::Bounds& bounds,
               const vtkm::Range& fieldRange,
               const vtkm::rendering::testing::RenderTestOptions& options)
{
  vtkm::rendering::Camera camera;
  // In a 1D view, the y bounds are determined by the field that is being x/y plotted.
  camera.ResetToBounds({ bounds.X, fieldRange, { 0, 0 } }, options.DataViewPadding);
  camera.SetClippingRange(options.ClippingRange);
  camera.SetViewport(options.Viewport);
  view.SetCamera(camera);

  view.SetLogX(options.LogX);
  view.SetLogY(options.LogY);
}

template <typename CanvasType, typename ViewType, typename MapperType>
void RenderTest(const DataSetFieldVector& dataSetsFields,
                const std::string& outputFile,
                const vtkm::rendering::testing::RenderTestOptions& options)
{
  CanvasType canvas(options.CanvasWidth, options.CanvasHeight);

  std::size_t numFields = dataSetsFields.size();
  VTKM_TEST_ASSERT(numFields > 0);

  vtkm::rendering::Scene scene;
  vtkm::Bounds bounds;
  vtkm::Range fieldRange;
  for (std::size_t dataFieldId = 0; dataFieldId < numFields; ++dataFieldId)
  {
    vtkm::cont::DataSet dataSet = dataSetsFields[dataFieldId].first;
    std::string fieldName = dataSetsFields[dataFieldId].second;
    if (options.Colors.empty())
    {
      scene.AddActor(vtkm::rendering::Actor(dataSet.GetCellSet(),
                                            dataSet.GetCoordinateSystem(),
                                            dataSet.GetField(fieldName),
                                            options.ColorTable));
    }
    else
    {
      scene.AddActor(vtkm::rendering::Actor(dataSet.GetCellSet(),
                                            dataSet.GetCoordinateSystem(),
                                            dataSet.GetField(fieldName),
                                            options.Colors[dataFieldId % options.Colors.size()]));
    }
    bounds.Include(dataSet.GetCoordinateSystem().GetBounds());
    fieldRange.Include(dataSet.GetField(fieldName).GetRange().ReadPortal().Get(0));
  }

  MapperType mapper;

  ViewType view(scene, mapper, canvas, options.Background, options.Foreground);
  SetupView(view, bounds, fieldRange, options);
  view.AddTextAnnotation(std::unique_ptr<vtkm::rendering::TextAnnotationScreen>(
    new vtkm::rendering::TextAnnotationScreen(options.Title,
                                              options.Foreground,
                                              options.TitleScale,
                                              options.TitlePosition,
                                              options.TitleAngle)));
  view.SetRenderAnnotationsEnabled(options.EnableAnnotations);

  VTKM_TEST_ASSERT(test_equal_images(view,
                                     outputFile,
                                     options.AverageRadius,
                                     options.PixelShiftRadius,
                                     options.AllowedPixelErrorRatio,
                                     options.Threshold));
}

template <typename CanvasType, typename ViewType>
void RenderTest(const DataSetFieldVector& dataSetsFields,
                const std::string& outputFile,
                const vtkm::rendering::testing::RenderTestOptions& options)
{
  switch (options.Mapper)
  {
    case vtkm::rendering::testing::MapperType::RayTracer:
      RenderTest<CanvasType, ViewType, vtkm::rendering::MapperRayTracer>(
        dataSetsFields, outputFile, options);
      break;
    case vtkm::rendering::testing::MapperType::Point:
      RenderTest<CanvasType, ViewType, vtkm::rendering::MapperPoint>(
        dataSetsFields, outputFile, options);
      break;
    case vtkm::rendering::testing::MapperType::Quad:
      RenderTest<CanvasType, ViewType, vtkm::rendering::MapperQuad>(
        dataSetsFields, outputFile, options);
      break;
    case vtkm::rendering::testing::MapperType::Volume:
      RenderTest<CanvasType, ViewType, vtkm::rendering::MapperVolume>(
        dataSetsFields, outputFile, options);
      break;
    case vtkm::rendering::testing::MapperType::Wireframer:
      RenderTest<CanvasType, ViewType, vtkm::rendering::MapperWireframer>(
        dataSetsFields, outputFile, options);
      break;
    default:
      VTKM_TEST_FAIL("Invalid mapper type for 3D view.");
  }
}

template <typename CanvasType>
void RenderTest(const DataSetFieldVector& dataSetsFields,
                const std::string& outputFile,
                const vtkm::rendering::testing::RenderTestOptions& options)
{
  switch (options.ViewDimension)
  {
    case 3:
      RenderTest<CanvasType, vtkm::rendering::View3D>(dataSetsFields, outputFile, options);
      break;
    case 2:
      RenderTest<CanvasType, vtkm::rendering::View2D>(dataSetsFields, outputFile, options);
      break;
    case 1:
      RenderTest<CanvasType, vtkm::rendering::View1D>(dataSetsFields, outputFile, options);
      break;
    default:
      VTKM_TEST_FAIL("Unsupported dimension for RenderTest.");
  }
}

} // annonymous namesapce

namespace vtkm
{
namespace rendering
{
namespace testing
{

void RenderTest(const vtkm::cont::DataSet& dataSet,
                const std::string& fieldName,
                const std::string& outputFile,
                const RenderTestOptions& options)
{
  RenderTest({ { dataSet, fieldName } }, outputFile, options);
}

void RenderTest(const DataSetFieldVector& dataSetsFields,
                const std::string& outputFile,
                const RenderTestOptions& options)
{
  std::unique_ptr<vtkm::cont::ScopedRuntimeDeviceTracker> deviceScope;
  if (options.AllowAnyDevice)
  {
    deviceScope =
      std::make_unique<vtkm::cont::ScopedRuntimeDeviceTracker>(vtkm::cont::DeviceAdapterTagAny{});
  }

  switch (options.Canvas)
  {
    case vtkm::rendering::testing::CanvasType::RayTracer:
      ::RenderTest<vtkm::rendering::CanvasRayTracer>(dataSetsFields, outputFile, options);
      break;
    default:
      VTKM_TEST_FAIL("Unknown canvas type specified in RenderTest.");
  }
}

} // namespace vtkm::rendering::testing
} // namespace vtkm::rendering
} // namespace vtkm
