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
#include <vtkm/rendering/MapperConnectivity.h>
#include <vtkm/rendering/MapperCylinder.h>
#include <vtkm/rendering/MapperGlyphScalar.h>
#include <vtkm/rendering/MapperGlyphVector.h>
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

template <typename ViewType>
std::unique_ptr<vtkm::rendering::View> MakeView(
  vtkm::rendering::Canvas& canvas,
  vtkm::rendering::Mapper& mapper,
  vtkm::rendering::Scene& scene,
  const vtkm::Bounds& bounds,
  const vtkm::Range& fieldRange,
  const vtkm::rendering::testing::RenderTestOptions& options)
{
  ViewType* view = new ViewType(scene, mapper, canvas, options.Background, options.Foreground);
  SetupView(*view, bounds, fieldRange, options);
  return std::unique_ptr<vtkm::rendering::View>(view);
}

template <typename MapperType>
void SetupMapper(MapperType&, const vtkm::rendering::testing::RenderTestOptions&)
{
}

void SetupMapper(vtkm::rendering::MapperCylinder& mapper,
                 const vtkm::rendering::testing::RenderTestOptions& options)
{
  mapper.UseVariableRadius(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetRadius(options.Radius);
  }
  mapper.SetRadiusDelta(0.5);
}

void SetupMapper(vtkm::rendering::MapperPoint& mapper,
                 const vtkm::rendering::testing::RenderTestOptions& options)
{
  mapper.UseVariableRadius(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetRadius(options.Radius);
  }
  mapper.SetRadiusDelta(0.5);
  if (options.RenderCells)
  {
    mapper.SetUseCells();
  }
}

void SetupMapper(vtkm::rendering::MapperGlyphScalar& mapper,
                 const vtkm::rendering::testing::RenderTestOptions& options)
{
  mapper.SetGlyphType(options.GlyphType);
  mapper.SetScaleByValue(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetBaseSize(options.Radius);
  }
  mapper.SetScaleDelta(0.5);
  if (options.RenderCells)
  {
    mapper.SetUseCells();
  }
}

void SetupMapper(vtkm::rendering::MapperGlyphVector& mapper,
                 const vtkm::rendering::testing::RenderTestOptions& options)
{
  mapper.SetGlyphType(options.GlyphType);
  mapper.SetScaleByValue(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetBaseSize(options.Radius);
  }
  mapper.SetScaleDelta(0.5);
  if (options.RenderCells)
  {
    mapper.SetUseCells();
  }
}

template <typename MapperType>
std::unique_ptr<vtkm::rendering::Mapper> MakeMapper(
  const vtkm::rendering::testing::RenderTestOptions& options)
{
  MapperType* mapper = new MapperType;
  SetupMapper(*mapper, options);
  return std::unique_ptr<vtkm::rendering::Mapper>(mapper);
}

void DoRenderTest(vtkm::rendering::Canvas& canvas,
                  vtkm::rendering::Mapper& mapper,
                  const DataSetFieldVector& dataSetsFields,
                  const std::string& outputFile,
                  const vtkm::rendering::testing::RenderTestOptions& options)
{
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

  std::unique_ptr<vtkm::rendering::View> viewPointer;
  switch (options.ViewDimension)
  {
    case 1:
      viewPointer =
        MakeView<vtkm::rendering::View1D>(canvas, mapper, scene, bounds, fieldRange, options);
      break;
    case 2:
      viewPointer =
        MakeView<vtkm::rendering::View2D>(canvas, mapper, scene, bounds, fieldRange, options);
      break;
    case 3:
      viewPointer =
        MakeView<vtkm::rendering::View3D>(canvas, mapper, scene, bounds, fieldRange, options);
      break;
  }
  vtkm::rendering::View& view = *viewPointer;

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

void DoRenderTest(vtkm::rendering::CanvasRayTracer& canvas,
                  const DataSetFieldVector& dataSetsFields,
                  const std::string& outputFile,
                  const vtkm::rendering::testing::RenderTestOptions& options)
{
  std::unique_ptr<vtkm::rendering::Mapper> mapper;
  switch (options.Mapper)
  {
    case vtkm::rendering::testing::MapperType::RayTracer:
      mapper = MakeMapper<vtkm::rendering::MapperRayTracer>(options);
      break;
    case vtkm::rendering::testing::MapperType::Connectivity:
      mapper = MakeMapper<vtkm::rendering::MapperConnectivity>(options);
      break;
    case vtkm::rendering::testing::MapperType::Cylinder:
      mapper = MakeMapper<vtkm::rendering::MapperCylinder>(options);
      break;
    case vtkm::rendering::testing::MapperType::Point:
      mapper = MakeMapper<vtkm::rendering::MapperPoint>(options);
      break;
    case vtkm::rendering::testing::MapperType::Quad:
      mapper = MakeMapper<vtkm::rendering::MapperQuad>(options);
      break;
    case vtkm::rendering::testing::MapperType::Volume:
      mapper = MakeMapper<vtkm::rendering::MapperVolume>(options);
      break;
    case vtkm::rendering::testing::MapperType::Wireframer:
      mapper = MakeMapper<vtkm::rendering::MapperWireframer>(options);
      break;
    case vtkm::rendering::testing::MapperType::GlyphScalar:
      mapper = MakeMapper<vtkm::rendering::MapperGlyphScalar>(options);
      break;
    case vtkm::rendering::testing::MapperType::GlyphVector:
      mapper = MakeMapper<vtkm::rendering::MapperGlyphVector>(options);
      break;
  }
  DoRenderTest(canvas, *mapper, dataSetsFields, outputFile, options);
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

  if (options.Canvas != vtkm::rendering::testing::CanvasType::RayTracer)
  {
    VTKM_TEST_FAIL("Currently only the CanvasRayTracer canvas is supported.");
  }

  vtkm::rendering::CanvasRayTracer canvas(options.CanvasWidth, options.CanvasHeight);
  DoRenderTest(canvas, dataSetsFields, outputFile, options);
}

} // namespace vtkm::rendering::testing
} // namespace vtkm::rendering
} // namespace vtkm
