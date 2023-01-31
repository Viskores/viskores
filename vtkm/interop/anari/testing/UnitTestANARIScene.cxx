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
#include <vtkm/interop/anari/ANARIMapperTriangles.h>
#include <vtkm/interop/anari/ANARIMapperVolume.h>
#include <vtkm/interop/anari/ANARIScene.h>
// vtk-m
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/contour/Contour.h>
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

  auto& tangle_field = tangle.GetField("tangle");
  vtkm::Range range;
  tangle_field.GetRange(&range);
  const auto isovalue = range.Center();

  vtkm::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValue(isovalue);
  contourFilter.SetActiveField(tangle_field.GetName());
  auto tangleIso = contourFilter.Execute(tangle);

  vtkm::filter::vector_analysis::Gradient gradientFilter;
  gradientFilter.SetActiveField(tangle_field.GetName());
  gradientFilter.SetOutputFieldName("Gradient");
  auto tangleGrad = gradientFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  vtkm::interop::anari::ANARIScene scene(d);

  auto& mVol = scene.AddMapper(vtkm::interop::anari::ANARIMapperVolume(d));
  mVol.SetName("volume");

  auto& mIso = scene.AddMapper(vtkm::interop::anari::ANARIMapperTriangles(d));
  mIso.SetName("isosurface");
  mIso.SetCalculateNormals(true);

  auto& mGrad = scene.AddMapper(vtkm::interop::anari::ANARIMapperGlyphs(d));
  mGrad.SetName("gradient");

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       scene.GetANARIWorld(),
                       vtkm::Vec3f_32(-0.05, 1.43, 1.87),
                       vtkm::Vec3f_32(0.32, -0.53, -0.79),
                       vtkm::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/scene-empty-mappers.png");

  // Render a frame ///////////////////////////////////////////////////////////

  mVol.SetActor({ tangle.GetCellSet(), tangle.GetCoordinateSystem(), tangle.GetField("tangle") });
  mIso.SetActor(
    { tangleIso.GetCellSet(), tangleIso.GetCoordinateSystem(), tangleIso.GetField("tangle") });
  mGrad.SetActor(
    { tangleGrad.GetCellSet(), tangleGrad.GetCoordinateSystem(), tangleGrad.GetField("Gradient") });

  setColorMap(d, mVol);
  setColorMap(d, mIso);
  setColorMap(d, mGrad);

  renderTestANARIImage(d,
                       scene.GetANARIWorld(),
                       vtkm::Vec3f_32(-0.05, 1.43, 1.87),
                       vtkm::Vec3f_32(0.32, -0.53, -0.79),
                       vtkm::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/scene.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIScene(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
