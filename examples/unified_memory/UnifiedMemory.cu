//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleStreaming.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Logging.h> //for GetHumanReadableSize
#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherStreamingMapField.h>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>

namespace vtkm
{
namespace worklet
{
class SineWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
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
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  vtkm::cont::Initialize(argc, argv, opts);

  vtkm::Int64 N = 4 * 512 * 512 * 512;
  if (argc > 1)
  {
    N = atoi(argv[1]);
  }

  std::cout << "Testing streaming worklet on "
            << vtkm::cont::GetHumanReadableSize(N * sizeof(vtkm::Int64)) << std::endl;

  std::vector<vtkm::Int64> data(N);
  for (vtkm::Int64 i = 0; i < N; i++)
    data[i] = i;

  using DeviceTag = vtkm::cont::DeviceAdapterTagCuda;
  const bool usingManagedMemory = vtkm::cont::cuda::internal::CudaAllocator::UsingManagedMemory();
  vtkm::worklet::SineWorklet sineWorklet;

  if (usingManagedMemory)
  {
    vtkm::cont::ArrayHandle<vtkm::Int64> input = vtkm::cont::make_ArrayHandle(data);
    vtkm::cont::ArrayHandle<vtkm::Float32> output;

    std::cout << "Testing with unified memory" << std::endl;
    vtkm::worklet::DispatcherMapField<vtkm::worklet::SineWorklet> dispatcher(sineWorklet);
    dispatcher.SetDevice(DeviceTag{});

    //run once to get the CUDA code warmed up
    dispatcher.Invoke(input, output);

    vtkm::cont::Timer timer{ DeviceTag() };
    timer.Start();

    for (int i = 0; i < 3; ++i)
    {
      dispatcher.Invoke(input, output);
      std::cout << output.GetPortalConstControl().Get(output.GetNumberOfValues() - 1) << std::endl;
    }

    vtkm::Float64 elapsedTime = timer.GetElapsedTime();
    std::cout << "Time for 3 iterations with managed memory: " << elapsedTime << std::endl;
  }

  if (usingManagedMemory)
  { //disable managed memory if it is enabled to get
    //the correct performance numbers on GPU's that support managed memory
    vtkm::cont::cuda::internal::CudaAllocator::ForceManagedMemoryOff();
  }

  vtkm::Id NBlocks = (N * sizeof(vtkm::Int64)) / (1 << 25);
  NBlocks = std::max(vtkm::Id(1), NBlocks);

  vtkm::worklet::DispatcherStreamingMapField<vtkm::worklet::SineWorklet> dispatcher(sineWorklet);
  dispatcher.SetNumberOfBlocks(NBlocks);

  vtkm::cont::ArrayHandle<vtkm::Int64> input = vtkm::cont::make_ArrayHandle(data);
  vtkm::cont::ArrayHandle<vtkm::Float32> output;

  std::cout << "Testing with streaming (without unified memory) with " << NBlocks << " blocks"
            << std::endl;

  //run once to get the CUDA code warmed up
  dispatcher.Invoke(input, output);

  vtkm::cont::Timer timer{ DeviceTag() };
  timer.Start();

  for (int i = 0; i < 3; ++i)
  {
    dispatcher.Invoke(input, output);
    std::cout << output.GetPortalConstControl().Get(output.GetNumberOfValues() - 1) << std::endl;
  }

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();
  std::cout << "Time for 3 iterations: " << elapsedTime << std::endl;


  return 0;
}
