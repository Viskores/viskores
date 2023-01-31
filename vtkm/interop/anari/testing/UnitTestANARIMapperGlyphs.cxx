//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// vtkm::anari
#include <vtkm/interop/anari/ANARIMapperGlyphs.h>
// vtk-m
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/vector_analysis/Gradient.h>
#include <vtkm/io/EncodePNG.h>
#include <vtkm/source/Tangle.h>
// std
#include <cstdlib>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  // Create VTKm datasets /////////////////////////////////////////////////////

  vtkm::source::Tangle source;
  source.SetPointDimensions({ 32 });
  auto tangle = source.Execute();

  vtkm::filter::vector_analysis::Gradient gradientFilter;
  gradientFilter.SetActiveField("tangle");
  gradientFilter.SetOutputFieldName("Gradient");
  auto tangleGrad = gradientFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  auto world = anari_cpp::newObject<anari_cpp::World>(d);

  vtkm::interop::anari::ANARIActor actor(
    tangleGrad.GetCellSet(), tangleGrad.GetCoordinateSystem(), tangleGrad.GetField("Gradient"));

  vtkm::interop::anari::ANARIMapperGlyphs mGlyphs(d, actor);

  auto surface = mGlyphs.GetANARISurface();
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       vtkm::Vec3f_32(0.5f, 1.f, 0.6f),
                       vtkm::Vec3f_32(0.f, -1.f, 0.f),
                       vtkm::Vec3f_32(0.f, 0.f, 1.f),
                       "interop/anari/glyphs.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIMapperGlyphs(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
