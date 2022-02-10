//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/source/PerlinNoise.h>

#include <vtkm/filter/contour/Contour.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{

void TestPerlinNoise()
{
  vtkm::source::PerlinNoise noiseSource(vtkm::Id3(16), 77698);
  vtkm::cont::DataSet noise = noiseSource.Execute();

  noise.PrintSummary(std::cout);

  vtkm::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValues({ 0.3, 0.4, 0.5, 0.6, 0.7 });
  contourFilter.SetActiveField("perlinnoise");
  vtkm::cont::DataSet contours = contourFilter.Execute(noise);

  // CUDA seems to make the contour slightly different, so relax comparison options.
  vtkm::rendering::testing::RenderTestOptions options;
  options.AllowedPixelErrorRatio = 0.01f;
  options.Threshold = 0.1f;

  vtkm::rendering::testing::RenderTest(contours, "perlinnoise", "source/perlin-noise.png", options);
}

} // anonymous namespace

int RenderTestPerlinNoise(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPerlinNoise, argc, argv);
}
