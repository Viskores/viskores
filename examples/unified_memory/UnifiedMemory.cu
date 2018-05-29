//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_CUDA

#include <vtkm/cont/ArrayHandleStreaming.h>
#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherStreamingMapField.h>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>

namespace
{

// Define the tangle field for the input data
class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<IdType> vertexId, FieldOut<Scalar> v);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  const vtkm::Id xdim, ydim, zdim;
  const vtkm::Float32 xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT
  TangleField(const vtkm::Id3 dims, const vtkm::Float32 mins[3], const vtkm::Float32 maxs[3])
    : xdim(dims[0])
    , ydim(dims[1])
    , zdim(dims[2])
    , xmin(mins[0])
    , ymin(mins[1])
    , zmin(mins[2])
    , xmax(maxs[0])
    , ymax(maxs[1])
    , zmax(maxs[2])
    , cellsPerLayer((xdim) * (ydim)){};

  VTKM_EXEC
  void operator()(const vtkm::Id& vertexId, vtkm::Float32& v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const vtkm::Float32 fx = static_cast<vtkm::Float32>(x) / static_cast<vtkm::Float32>(xdim - 1);
    const vtkm::Float32 fy = static_cast<vtkm::Float32>(y) / static_cast<vtkm::Float32>(xdim - 1);
    const vtkm::Float32 fz = static_cast<vtkm::Float32>(z) / static_cast<vtkm::Float32>(xdim - 1);

    const vtkm::Float32 xx = 3.0f * (xmin + (xmax - xmin) * (fx));
    const vtkm::Float32 yy = 3.0f * (ymin + (ymax - ymin) * (fy));
    const vtkm::Float32 zz = 3.0f * (zmin + (zmax - zmin) * (fz));

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};

// Construct an input data set using the tangle field worklet
vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  vtkm::Float32 mins[3] = { -1.0f, -1.0f, -1.0f };
  vtkm::Float32 maxs[3] = { 1.0f, 1.0f, 1.0f };

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> vertexCountImplicitArray(
    0, 1, vdims[0] * vdims[1] * vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(
    TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, fieldArray);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(1.0f / static_cast<vtkm::FloatDefault>(dims[0]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[2]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  dataSet.AddField(
    vtkm::cont::Field("nodevar", vtkm::cont::Field::Association::POINTS, fieldArray));

  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}
}

namespace vtkm
{
namespace worklet
{
class SineWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<>, FieldOut<>);
  using ExecutionSignature = _2(_1, WorkIndex);

  VTKM_EXEC
  vtkm::Float32 operator()(vtkm::Int64 x, vtkm::Id&) const
  {
    return (vtkm::Sin(static_cast<vtkm::Float32>(x)));
  }
};
}
}

// Run a simple worklet, and compute an isosurface
int main(int argc, char* argv[])
{
  vtkm::Int64 N = 1024 * 1024 * 1024;
  if (argc > 1)
    N = N * atoi(argv[1]);
  else
    N = N * 4;
  std::cout << "Testing streaming worklet with size " << N << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Int64> input;
  vtkm::cont::ArrayHandle<vtkm::Float32> output;
  std::vector<vtkm::Int64> data(N);
  for (vtkm::Int64 i = 0; i < N; i++)
    data[i] = i;
  input = vtkm::cont::make_ArrayHandle(data);

  using DeviceAlgorithms = vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>;
  vtkm::worklet::SineWorklet sineWorklet;

  bool usingManagedMemory = vtkm::cont::cuda::internal::CudaAllocator::UsingManagedMemory();

  if (usingManagedMemory)
  {
    std::cout << "Testing with unified memory" << std::endl;

    vtkm::worklet::DispatcherMapField<vtkm::worklet::SineWorklet> dispatcher(sineWorklet);

    vtkm::cont::Timer<> timer;

    dispatcher.Invoke(input, output);
    std::cout << output.GetPortalConstControl().Get(output.GetNumberOfValues() - 1) << std::endl;

    vtkm::Float64 elapsedTime = timer.GetElapsedTime();
    std::cout << "Time: " << elapsedTime << std::endl;
  }
  else
  {
    vtkm::worklet::DispatcherStreamingMapField<vtkm::worklet::SineWorklet> dispatcher(sineWorklet);
    vtkm::Id NBlocks = N / (1024 * 1024 * 1024);
    NBlocks *= 2;
    dispatcher.SetNumberOfBlocks(NBlocks);
    std::cout << "Testing with streaming (without unified memory) with " << NBlocks << " blocks"
              << std::endl;

    vtkm::cont::Timer<> timer;

    dispatcher.Invoke(input, output);
    std::cout << output.GetPortalConstControl().Get(output.GetNumberOfValues() - 1) << std::endl;

    vtkm::Float64 elapsedTime = timer.GetElapsedTime();
    std::cout << "Time: " << elapsedTime << std::endl;
  }

  int dim = 128;
  if (argc > 2)
    dim = atoi(argv[2]);
  std::cout << "Testing Marching Cubes with size " << dim << "x" << dim << "x" << dim << std::endl;

  vtkm::Id3 dims(dim, dim, dim);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> verticesArray, normalsArray;
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;
  vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

  vtkm::filter::MarchingCubes filter;
  filter.SetGenerateNormals(true);
  filter.SetMergeDuplicatePoints(false);
  filter.SetActiveField("nodevar");
  filter.SetIsoValue(0.5);
  auto outputData = filter.Execute(dataSet);

  //need to extract vertices, normals, and scalars
  using VertType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>;
  vtkm::cont::CoordinateSystem coords = outputData.GetCoordinateSystem();

  verticesArray = coords.GetData().Cast<VertType>();
  normalsArray = outputData.GetField("normals").GetData().Cast<VertType>();
  scalarsArray =
    outputData.GetField("nodevar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  std::cout << "Number of output vertices: " << verticesArray.GetNumberOfValues() << std::endl;

  std::cout << "vertices: ";
  vtkm::cont::printSummary_ArrayHandle(verticesArray, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  vtkm::cont::printSummary_ArrayHandle(normalsArray, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  vtkm::cont::printSummary_ArrayHandle(scalarsArray, std::cout);
  std::cout << std::endl;

  return 0;
}
