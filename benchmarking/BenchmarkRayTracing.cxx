//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include "Benchmarker.h"

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayTracer.h>

#include <vtkm/exec/FunctorBase.h>

#include <sstream>
#include <string>
#include <vector>

using namespace vtkm::benchmarking;
namespace vtkm
{
namespace benchmarking
{

template <typename Precision>
struct BenchRayTracing
{
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> Indices;
  vtkm::rendering::raytracing::Ray<Precision> Rays;
  vtkm::Id NumberOfTriangles;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::cont::DataSet Data;

  VTKM_CONT BenchRayTracing()
  {
    vtkm::cont::testing::MakeTestDataSet maker;
    Data = maker.Make3DUniformDataSet2();
    Coords = Data.GetCoordinateSystem();

    vtkm::rendering::Camera camera;
    vtkm::Bounds bounds = Data.GetCoordinateSystem().GetBounds();
    camera.ResetToBounds(bounds);

    vtkm::cont::DynamicCellSet cellset = Data.GetCellSet();
    vtkm::rendering::internal::RunTriangulator(cellset, Indices, NumberOfTriangles);

    vtkm::rendering::CanvasRayTracer canvas(1920, 1080);
    RayCamera.SetParameters(camera, canvas);
    RayCamera.CreateRays(Rays, Coords);

    Rays.Buffers.at(0).InitConst(0.f);

    vtkm::cont::Field field = Data.GetField("pointvar");
    vtkm::Range range = field.GetRange().GetPortalConstControl().Get(0);

    Tracer.SetData(Coords.GetData(), Indices, field, NumberOfTriangles, range, bounds);

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
    vtkm::cont::Timer<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> timer;

    RayCamera.CreateRays(Rays, Coords);
    Tracer.Render(Rays);

    return timer.GetElapsedTime();
  }

  VTKM_CONT
  std::string Description() const { return "A ray tracing benchmark"; }
};

VTKM_MAKE_BENCHMARK(RayTracing, BenchRayTracing);
}
} // end namespace vtkm::benchmarking

int main(int, char* [])
{
  VTKM_RUN_BENCHMARK(RayTracing, vtkm::ListTagBase<vtkm::Float32>());
  return 0;
}
