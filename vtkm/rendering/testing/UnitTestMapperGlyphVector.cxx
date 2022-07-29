//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/GlyphType.h>
#include <vtkm/rendering/MapperPoint.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::GlyphVector;
  options.AllowAnyDevice = false;
  options.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;
  options.GlyphType = vtkm::rendering::GlyphType::Arrow;
  options.UseVariableRadius = true;
  options.RadiusDelta = 4.0f;
  options.Radius = 0.02f;

  vtkm::rendering::testing::RenderTest(maker.Make3DExplicitDataSetCowNose(),
                                       "point_vectors",
                                       "rendering/glyph_vector/points_arrows_cownose.png",
                                       options);
}

} //namespace

int UnitTestMapperGlyphVector(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
