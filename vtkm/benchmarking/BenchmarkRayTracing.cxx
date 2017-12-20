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

#include <vtkm/benchmarking/Benchmarker.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/rendering/Camera.h>
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
  // Setup anything that doesn't need to change per run in the constructor
  VTKM_CONT BenchRayTracing()
  {
    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::cont::DataSet data = maker.Make3DUniformDataSet2();

    vtkm::rendering::Camera camera;
    vtkm::Bounds bounds = data.GetCoordinateSystem().GetBounds();
    camera.ResetToBounds(bounds);
  }

  // The overloaded call operator will run the operations being timed and
  // return the execution time
  VTKM_CONT
  vtkm::Float64 operator()() { return 0.05; }

  // The benchmark must also provide a method describing itself, this is
  // used when printing out run time statistics
  VTKM_CONT
  std::string Description() const { return "A ray tracing benchmark"; }
};

// Now use the VTKM_MAKE_BENCHMARK macro to generate a maker functor for
// your benchmark. This lets us generate the benchmark functor for each type
// we want to test
VTKM_MAKE_BENCHMARK(RayTracing, BenchRayTracing);
}
} // end namespace vtkm::benchmarking

int main(int, char* [])
{
  using Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  //using Benchmarks = vtkm::benchmarking::BenchRayTracing<Device>;
  using TestTypes = vtkm::ListTagBase<vtkm::Float32>;
  VTKM_RUN_BENCHMARK(RayTracing, vtkm::ListTagBase<vtkm::Float32>());
  //bool result = Benchmarks::Run();
  //return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
