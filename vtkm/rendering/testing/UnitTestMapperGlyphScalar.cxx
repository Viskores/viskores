//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/GlyphType.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::GlyphScalar;
  options.AllowAnyDevice = false;
  options.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;

  options.GlyphType = vtkm::rendering::GlyphType::Cube;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DUniformDataSet1(), "pointvar", "rendering/glyph_scalar/regular3D.png", options);

  options.UseVariableRadius = true;
  options.RadiusDelta = 4.0f;
  options.Radius = 0.25f;
  vtkm::rendering::testing::RenderTest(maker.Make3DUniformDataSet1(),
                                       "pointvar",
                                       "rendering/glyph_scalar/variable_regular3D.png",
                                       options);

  options.GlyphType = vtkm::rendering::GlyphType::Sphere;
  vtkm::rendering::testing::RenderTest(maker.Make3DUniformDataSet3({ 7 }),
                                       "pointvar",
                                       "rendering/glyph_scalar/variable_spheres_regular3D.png",
                                       options);

  options.GlyphType = vtkm::rendering::GlyphType::Axes;
  vtkm::rendering::testing::RenderTest(maker.Make3DUniformDataSet3({ 7 }),
                                       "pointvar",
                                       "rendering/glyph_scalar/variable_axes_regular3D.png",
                                       options);

  options.GlyphType = vtkm::rendering::GlyphType::Quad;
  options.Radius = 5.0f;
  options.RadiusDelta = 0.75f;
  vtkm::rendering::testing::RenderTest(maker.Make3DUniformDataSet3({ 7 }),
                                       "pointvar",
                                       "rendering/glyph_scalar/variable_quads_regular3D.png",
                                       options);

  // restore defaults
  options.RadiusDelta = 0.5f;
  options.UseVariableRadius = false;
  options.GlyphType = vtkm::rendering::GlyphType::Cube;

  options.RenderCells = true;
  options.Radius = 1.f;
  vtkm::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet7(), "cellvar", "rendering/glyph_scalar/cells.png", options);
}

} //namespace

int UnitTestMapperGlyphScalar(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
