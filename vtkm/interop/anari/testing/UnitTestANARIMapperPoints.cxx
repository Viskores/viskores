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
#include <vtkm/interop/anari/ANARIMapperPoints.h>
// vtk-m
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/contour/Contour.h>
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

  auto& tangle_field = tangle.GetField("tangle");
  vtkm::Range range;
  tangle_field.GetRange(&range);
  const auto isovalue = range.Center();

  vtkm::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValue(isovalue);
  contourFilter.SetActiveField("tangle");
  auto tangleIso = contourFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  auto world = anari_cpp::newObject<anari_cpp::World>(d);

  vtkm::interop::anari::ANARIActor actor(
    tangleIso.GetCellSet(), tangleIso.GetCoordinateSystem(), tangleIso.GetField("tangle"));

  vtkm::interop::anari::ANARIMapperPoints mIso(d, actor);
  setColorMap(d, mIso);

  auto surface = mIso.GetANARISurface();
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);

  anari_cpp::commitParameters(d, world);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       vtkm::Vec3f_32(-0.05, 1.43, 1.87),
                       vtkm::Vec3f_32(0.32, -0.53, -0.79),
                       vtkm::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/points.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIMapperPoints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
