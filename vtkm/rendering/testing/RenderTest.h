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

#include <vtkm/rendering/testing/vtkm_rendering_testing_export.h>

#include <vtkm/Bounds.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/GlyphType.h>
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

enum struct CanvasType
{
  RayTracer
};

enum struct MapperType
{
  RayTracer,
  Connectivity,
  Cylinder,
  Point,
  Quad,
  Volume,
  Wireframer,
  GlyphScalar,
  GlyphVector
};

struct RenderTestOptions
{
  // Options for comparing images (i.e. test_equal_images)
  vtkm::IdComponent AverageRadius = 0;
  vtkm::IdComponent PixelShiftRadius = 0;
  vtkm::FloatDefault AllowedPixelErrorRatio = 0.00025f;
  vtkm::FloatDefault Threshold = 0.05f;

  // Options that set up rendering
  CanvasType Canvas = CanvasType::RayTracer;
  vtkm::IdComponent ViewDimension = 3;
  MapperType Mapper = MapperType::RayTracer;
  vtkm::Id CanvasWidth = 512;
  vtkm::Id CanvasHeight = 512;
  bool EnableAnnotations = true;
  vtkm::Float64 DataViewPadding = 0;
  vtkm::rendering::Color Foreground = vtkm::rendering::Color::black;
  vtkm::rendering::Color Background = vtkm::rendering::Color::white;

  // By default, scalar values will be mapped by this ColorTable to make colors.
  vtkm::cont::ColorTable ColorTable;
  // If you want constant colors (per DataSet or field or partition), then you can
  // set this vector to the colors you want to use. If one color is specified, it
  // will be used for everything. If multiple colors are specified, each will be
  // used for a different DataSet/field/partition.
  std::vector<vtkm::rendering::Color> Colors;

  // For 3D rendering
  vtkm::Float32 CameraAzimuth = 45.0f;
  vtkm::Float32 CameraElevation = 45.0f;

  // For 2D/1D rendering
  vtkm::Range ClippingRange = { 1, 100 };
  vtkm::Bounds Viewport = { { -0.7, 0.7 }, { -0.7, 0.7 }, { 0.0, 0.0 } };

  // For 1D rendering
  bool LogX = false;
  bool LogY = false;

  std::string Title = "";
  vtkm::Float32 TitleScale = 0.075f;
  vtkm::Vec2f_32 TitlePosition = { -0.11f, 0.92f };
  vtkm::Float32 TitleAngle = 0;

  // Usually when calling RenderTest, you are not specifically testing rendering.
  // Rather, you are testing something else and using render to check the results.
  // Regardless of what device you are using for testing, you probably want to
  // use the best available device for rendering.
  bool AllowAnyDevice = true;

  // Special options for some glyph and glyph-like mappers
  vtkm::rendering::GlyphType GlyphType = vtkm::rendering::GlyphType::Cube;
  bool UseVariableRadius = false;
  vtkm::Float32 Radius = -1.0f;
  vtkm::Float32 RadiusDelta = 0.5f;
  bool RenderCells = false;
};

VTKM_RENDERING_TESTING_EXPORT
void RenderTest(const vtkm::cont::DataSet& dataSet,
                const std::string& fieldName,
                const std::string& outputFile,
                const RenderTestOptions& options = RenderTestOptions{});

VTKM_RENDERING_TESTING_EXPORT
void RenderTest(const std::vector<std::pair<vtkm::cont::DataSet, std::string>>& dataSetsFields,
                const std::string& outputFile,
                const RenderTestOptions& options = RenderTestOptions{});

} // namespace vtkm::rendering::testing
} // namespace vtkm::rendering
} // namespace vtkm

#endif //vtk_m_rendering_testing_RenderTest_h
