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

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/SphereIntersector.h>
#include <vtkm/rendering/raytracing/TriangleExtractor.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/ColorTable.hxx>

#include <sstream>
#include <string>
#include <vector>

namespace
{

// Hold configuration state (e.g. active device)
vtkm::cont::InitializeResult Config;

void BenchRayTracing(::benchmark::State& state)
{
  const vtkm::Id3 dims(128, 128, 128);

  vtkm::cont::testing::MakeTestDataSet maker;
  auto dataset = maker.Make3DUniformDataSet3(dims);
  auto coords = dataset.GetCoordinateSystem();

  vtkm::rendering::Camera camera;
  vtkm::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();
  camera.ResetToBounds(bounds);

  vtkm::cont::DynamicCellSet cellset = dataset.GetCellSet();

  vtkm::rendering::raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cellset);

  auto triIntersector = std::make_shared<vtkm::rendering::raytracing::TriangleIntersector>(
    vtkm::rendering::raytracing::TriangleIntersector());

  vtkm::rendering::raytracing::RayTracer tracer;
  triIntersector->SetData(coords, triExtractor.GetTriangles());
  tracer.AddShapeIntersector(triIntersector);

  vtkm::rendering::CanvasRayTracer canvas(1920, 1080);
  vtkm::rendering::raytracing::Camera rayCamera;
  rayCamera.SetParameters(camera, canvas);
  vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;
  rayCamera.CreateRays(rays, coords.GetBounds());

  rays.Buffers.at(0).InitConst(0.f);

  vtkm::cont::Field field = dataset.GetField("pointvar");
  vtkm::Range range = field.GetRange().GetPortalConstControl().Get(0);

  tracer.SetField(field, range);

  vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> temp;
  vtkm::cont::ColorTable table("cool to warm");
  table.Sample(100, temp);

  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> colors;
  colors.Allocate(100);
  auto portal = colors.GetPortalControl();
  auto colorPortal = temp.GetPortalConstControl();
  constexpr vtkm::Float32 conversionToFloatSpace = (1.0f / 255.0f);
  for (vtkm::Id i = 0; i < 100; ++i)
  {
    auto color = colorPortal.Get(i);
    vtkm::Vec4f_32 t(color[0] * conversionToFloatSpace,
                     color[1] * conversionToFloatSpace,
                     color[2] * conversionToFloatSpace,
                     color[3] * conversionToFloatSpace);
    portal.Set(i, t);
  }

  tracer.SetColorMap(colors);
  tracer.Render(rays);

  vtkm::cont::Timer timer{ Config.Device };
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

VTKM_BENCHMARK(BenchRayTracing);

} // end namespace vtkm::benchmarking

int main(int argc, char* argv[])
{
  // Parse VTK-m options:
  auto opts = vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::AddHelp;
  Config = vtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  VTKM_EXECUTE_BENCHMARKS(argc, argv);
}
