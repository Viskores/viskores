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

using namespace vtkm::benchmarking;
namespace vtkm
{
namespace benchmarking
{

template <typename Precision, typename DeviceAdapter>
struct BenchRayTracing
{
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> Indices;
  vtkm::rendering::raytracing::Ray<Precision> Rays;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::DataSet Data;

  VTKM_CONT ~BenchRayTracing() {}

  VTKM_CONT BenchRayTracing()
  {
    vtkm::Id3 dims(128, 128, 128);
    vtkm::cont::testing::MakeTestDataSet maker;
    Data = maker.Make3DUniformDataSet3(dims);
    Coords = Data.GetCoordinateSystem();

    vtkm::rendering::Camera camera;
    vtkm::Bounds bounds = Data.GetCoordinateSystem().GetBounds();
    camera.ResetToBounds(bounds);

    vtkm::cont::DynamicCellSet cellset = Data.GetCellSet();

    vtkm::rendering::raytracing::TriangleExtractor triExtractor;
    triExtractor.ExtractCells(cellset);

    auto triIntersector = std::make_shared<vtkm::rendering::raytracing::TriangleIntersector>(
      vtkm::rendering::raytracing::TriangleIntersector());

    triIntersector->SetData(Coords, triExtractor.GetTriangles());
    Tracer.AddShapeIntersector(triIntersector);

    vtkm::rendering::CanvasRayTracer canvas(1920, 1080);
    RayCamera.SetParameters(camera, canvas);
    RayCamera.CreateRays(Rays, Coords.GetBounds());

    Rays.Buffers.at(0).InitConst(0.f);

    vtkm::cont::Field field = Data.GetField("pointvar");
    vtkm::Range range = field.GetRange().GetPortalConstControl().Get(0);

    Tracer.SetField(field, range);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>> temp;
    vtkm::cont::ColorTable table("cool to warm");
    table.Sample(100, temp);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> colors;
    colors.Allocate(100);
    auto portal = colors.GetPortalControl();
    auto colorPortal = temp.GetPortalConstControl();
    constexpr vtkm::Float32 conversionToFloatSpace = (1.0f / 255.0f);
    for (vtkm::Id i = 0; i < 100; ++i)
    {
      auto color = colorPortal.Get(i);
      vtkm::Vec<vtkm::Float32, 4> t(color[0] * conversionToFloatSpace,
                                    color[1] * conversionToFloatSpace,
                                    color[2] * conversionToFloatSpace,
                                    color[3] * conversionToFloatSpace);
      portal.Set(i, t);
    }

    Tracer.SetColorMap(colors);
    Tracer.Render(Rays);
  }

  VTKM_CONT
  vtkm::Float64 operator()()
  {
    vtkm::cont::Timer timer{ DeviceAdapter() };
    timer.Start();

    RayCamera.CreateRays(Rays, Coords.GetBounds());
    try
    {
      Tracer.Render(Rays);
    }
    catch (vtkm::cont::ErrorBadValue& e)
    {
      std::cout << "exception " << e.what() << "\n";
    }

    return timer.GetElapsedTime();
  }

  VTKM_CONT
  std::string Description() const { return "A ray tracing benchmark"; }
};

VTKM_MAKE_BENCHMARK(RayTracing, BenchRayTracing);
}
} // end namespace vtkm::benchmarking


int main(int argc, char* argv[])
{
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  VTKM_RUN_BENCHMARK(RayTracing, vtkm::ListTagBase<vtkm::Float32>(), config.Device);
  return 0;
}
