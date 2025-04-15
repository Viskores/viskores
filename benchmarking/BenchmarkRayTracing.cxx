//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "Benchmarker.h"

#include <viskores/TypeTraits.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/Timer.h>

#include <viskores/source/Tangle.h>

#include <viskores/rendering/Camera.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{

// Hold configuration state (e.g. active device)
viskores::cont::InitializeResult Config;

void BenchRayTracing(::benchmark::State& state)
{
  viskores::source::Tangle maker;
  maker.SetPointDimensions({ 128, 128, 128 });
  viskores::cont::DataSet dataset = maker.Execute();
  viskores::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();

  viskores::rendering::Camera camera;
  viskores::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();
  camera.ResetToBounds(bounds);

  viskores::cont::UnknownCellSet cellset = dataset.GetCellSet();

  viskores::rendering::raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cellset);

  auto triIntersector = std::make_shared<viskores::rendering::raytracing::TriangleIntersector>(
    viskores::rendering::raytracing::TriangleIntersector());

  viskores::rendering::raytracing::RayTracer tracer;
  triIntersector->SetData(coords, triExtractor.GetTriangles());
  tracer.AddShapeIntersector(triIntersector);

  viskores::rendering::CanvasRayTracer canvas(1920, 1080);
  viskores::rendering::raytracing::Camera rayCamera;
  rayCamera.SetParameters(
    camera, viskores::Int32(canvas.GetWidth()), viskores::Int32(canvas.GetHeight()));
  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  rayCamera.CreateRays(rays, coords.GetBounds());

  rays.Buffers.at(0).InitConst(0.f);

  viskores::cont::Field field = dataset.GetField("tangle");
  viskores::Range range = field.GetRange().ReadPortal().Get(0);

  tracer.SetField(field, range);

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> temp;
  viskores::cont::ColorTable table("cool to warm");
  table.Sample(100, temp);

  viskores::cont::ArrayHandle<viskores::Vec4f_32> colors;
  colors.Allocate(100);
  auto portal = colors.WritePortal();
  auto colorPortal = temp.ReadPortal();
  constexpr viskores::Float32 conversionToFloatSpace = (1.0f / 255.0f);
  for (viskores::Id i = 0; i < 100; ++i)
  {
    auto color = colorPortal.Get(i);
    viskores::Vec4f_32 t(color[0] * conversionToFloatSpace,
                         color[1] * conversionToFloatSpace,
                         color[2] * conversionToFloatSpace,
                         color[3] * conversionToFloatSpace);
    portal.Set(i, t);
  }

  tracer.SetColorMap(colors);
  tracer.Render(rays);

  viskores::cont::Timer timer{ Config.Device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    rayCamera.CreateRays(rays, coords.GetBounds());
    tracer.Render(rays);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

VISKORES_BENCHMARK(BenchRayTracing);

} // end namespace viskores::benchmarking

int main(int argc, char* argv[])
{
  auto opts = viskores::cont::InitializeOptions::RequireDevice;

  std::vector<char*> args(argv, argv + argc);
  viskores::bench::detail::InitializeArgs(&argc, args, opts);

  // Parse Viskores options:
  Config = viskores::cont::Initialize(argc, args.data(), opts);

  // This occurs when it is help
  if (opts == viskores::cont::InitializeOptions::None)
  {
    std::cout << Config.Usage << std::endl;
  }
  else
  {
    viskores::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }

  // handle benchmarking related args and run benchmarks:
  VISKORES_EXECUTE_BENCHMARKS(argc, args.data());
}
