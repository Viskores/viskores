//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/filter/NewFilter.h>
#include <vtkm/filter/PolicyDefault.h>
#include <vtkm/filter/TaskQueue.h>

#include <future>

namespace vtkm
{
namespace filter
{
namespace
{
void RunFilter(NewFilter* self,
               vtkm::filter::DataSetQueue& input,
               vtkm::filter::DataSetQueue& output)
{
  std::pair<vtkm::Id, vtkm::cont::DataSet> task;
  while (input.GetTask(task))
  {
    auto outDS = self->Execute(task.second);
    output.Push(std::make_pair(task.first, std::move(outDS)));
  }

  vtkm::cont::Algorithm::Synchronize();
}
} // anonymous namespace

//----------------------------------------------------------------------------
vtkm::cont::PartitionedDataSet NewFilter::DoExecute(const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::PartitionedDataSet output;

  if (this->GetRunMultiThreadedFilter())
  {
    vtkm::filter::DataSetQueue inputQueue(input);
    vtkm::filter::DataSetQueue outputQueue;

    vtkm::Id numThreads = this->DetermineNumberOfThreads(input);

    //Run 'numThreads' filters.
    std::vector<std::future<void>> futures(static_cast<std::size_t>(numThreads));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numThreads); i++)
    {
      auto f = std::async(
        std::launch::async, RunFilter, this, std::ref(inputQueue), std::ref(outputQueue));
      futures[i] = std::move(f);
    }

    for (auto& f : futures)
      f.get();

    //Get results from the outputQueue.
    output = outputQueue.Get();
  }
  else
  {
    for (const auto& inBlock : input)
    {
      vtkm::cont::DataSet outBlock = this->Execute(inBlock);
      output.AppendPartition(outBlock);
    }
  }

  return output;
}

vtkm::cont::PartitionedDataSet NewFilter::Execute(const vtkm::cont::PartitionedDataSet& input)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                 "NewFilter (%d partitions): '%s'",
                 (int)input.GetNumberOfPartitions(),
                 vtkm::cont::TypeToString<decltype(*this)>().c_str());

  // Call `void Derived::PreExecute<DerivedPolicy>(input, policy)`, if defined.
  this->PreExecute(input);

  // Call `PrepareForExecution` (which should probably be renamed at some point)
  vtkm::cont::PartitionedDataSet output = this->DoExecute(input);

  // Call `Derived::PostExecute<DerivedPolicy>(input, output, policy)` if defined.
  this->PostExecute(input, output);

  return output;
}

vtkm::Id NewFilter::DetermineNumberOfThreads(const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::Id numDS = input.GetNumberOfPartitions();

  //Aribitrary constants.
  const vtkm::Id threadsPerGPU = 8;
  const vtkm::Id threadsPerCPU = 4;

  vtkm::Id availThreads = 1;

  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();

  if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda{}))
    availThreads = threadsPerGPU;
  else if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagKokkos{}))
  {
    //Kokkos doesn't support threading on the CPU.
#ifdef VTKM_KOKKOS_CUDA
    availThreads = threadsPerGPU;
#else
    availThreads = 1;
#endif
  }
  else if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagSerial{}))
    availThreads = 1;
  else
    availThreads = threadsPerCPU;

  vtkm::Id numThreads = std::min<vtkm::Id>(numDS, availThreads);
  return numThreads;
}

} // namespace filter
} // namespace vtkm
