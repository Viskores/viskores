//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "MultiDeviceGradient.h"

#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/openmp/DeviceAdapterOpenMP.h>
#include <viskores/cont/tbb/DeviceAdapterTBB.h>

#include <viskores/filter/vector_analysis/Gradient.h>

namespace
{

void process_partition_tbb(RuntimeTaskQueue& queue)
{
  //Step 1. Set the device adapter to this thread to TBB.
  //This makes sure that any viskores::filters used by our
  //task operate only on TBB. The "global" thread tracker
  //is actually thread-local, so we can use that.
  //
  viskores::cont::GetRuntimeDeviceTracker().ForceDevice(viskores::cont::DeviceAdapterTagTBB{});

  while (queue.hasTasks())
  {
    //Step 2. Get the task to run on TBB
    auto task = queue.pop();

    //Step 3. Run the task on TBB. We check the validity
    //of the task since we could be given an empty task
    //when the queue is empty and we are shutting down
    if (task != nullptr)
    {
      task();
    }

    //Step 4. Notify the queue that we finished processing this task
    queue.completedTask();
    std::cout << "finished a partition on tbb (" << std::this_thread::get_id() << ")" << std::endl;
  }
}

void process_partition_openMP(RuntimeTaskQueue& queue)
{
  //Step 1. Set the device adapter to this thread to openMP.
  //This makes sure that any viskores::filters used by our
  //task operate only on openMP. The "global" thread tracker
  //is actually thread-local, so we can use that.
  //
  viskores::cont::GetRuntimeDeviceTracker().ForceDevice(viskores::cont::DeviceAdapterTagOpenMP{});

  while (queue.hasTasks())
  {
    //Step 2. Get the task to run on openMP
    auto task = queue.pop();

    //Step 3. Run the task on openMP. We check the validity
    //of the task since we could be given an empty task
    //when the queue is empty and we are shutting down
    if (task != nullptr)
    {
      task();
    }

    //Step 4. Notify the queue that we finished processing this task
    queue.completedTask();
    std::cout << "finished a partition on openMP (" << std::this_thread::get_id() << ")"
              << std::endl;
  }
}

void process_partition_cuda(RuntimeTaskQueue& queue, int gpuId)
{
  //Step 1. Set the device adapter to this thread to cuda.
  //This makes sure that any viskores::filters used by our
  //task operate only on cuda.  The "global" thread tracker
  //is actually thread-local, so we can use that.
  //
  viskores::cont::GetRuntimeDeviceTracker().ForceDevice(viskores::cont::DeviceAdapterTagCuda{});
  (void)gpuId;

  while (queue.hasTasks())
  {
    //Step 2. Get the task to run on cuda
    auto task = queue.pop();

    //Step 3. Run the task on cuda. We check the validity
    //of the task since we could be given an empty task
    //when the queue is empty and we are shutting down
    if (task != nullptr)
    {
      task();
    }

    //Step 4. Notify the queue that we finished processing this task
    queue.completedTask();
    std::cout << "finished a partition on cuda (" << std::this_thread::get_id() << ")" << std::endl;
  }
}

} //namespace

//-----------------------------------------------------------------------------
VISKORES_CONT MultiDeviceGradient::MultiDeviceGradient()
  : ComputePointGradient(false)
  , Queue()
  , Workers()
{
  //Step 1. Determine the number of workers we want
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  const bool runOnCuda = tracker.CanRunOn(viskores::cont::DeviceAdapterTagCuda{});
  const bool runOnOpenMP = tracker.CanRunOn(viskores::cont::DeviceAdapterTagOpenMP{});
  const bool runOnTbb = tracker.CanRunOn(viskores::cont::DeviceAdapterTagTBB{});

  //Note currently the virtual implementation has some issues
  //In a multi-threaded environment only cuda can be used or
  //all SMP backends ( Serial, TBB, OpenMP ).
  //Once this issue is resolved we can enable CUDA + TBB in
  //this example

  //Step 2. Launch workers that will use cuda (if enabled).
  //The threads share a queue object so we need to explicitly pass it
  //by reference (the std::ref call)
  if (runOnCuda)
  {
    std::cout << "adding cuda workers" << std::endl;
    try
    {
      viskores::Id gpu_count = 0;
      viskores::cont::RuntimeDeviceInformation{}
        .GetRuntimeConfiguration(viskores::cont::DeviceAdapterTagCuda{})
        .GetMaxDevices(gpu_count);
      for (int i = 0; i < gpu_count; ++i)
      {
        //The number of workers per GPU is purely arbitrary currently,
        //but in general we want multiple of them so we can overlap compute
        //and transfer
        this->Workers.emplace_back(std::bind(process_partition_cuda, std::ref(this->Queue), i));
        this->Workers.emplace_back(std::bind(process_partition_cuda, std::ref(this->Queue), i));
        this->Workers.emplace_back(std::bind(process_partition_cuda, std::ref(this->Queue), i));
        this->Workers.emplace_back(std::bind(process_partition_cuda, std::ref(this->Queue), i));
      }
    }
    catch (const viskores::cont::ErrorBadDevice& err)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                 "Error getting CudaDeviceCount: " << err.GetMessage());
    }
  }
  //Step 3. Launch a worker that will use openMP (if enabled).
  //The threads share a queue object so we need to explicitly pass it
  //by reference (the std::ref call)
  else if (runOnOpenMP)
  {
    std::cout << "adding a openMP worker" << std::endl;
    this->Workers.emplace_back(std::bind(process_partition_openMP, std::ref(this->Queue)));
  }
  //Step 4. Launch a worker that will use tbb (if enabled).
  //The threads share a queue object so we need to explicitly pass it
  //by reference (the std::ref call)
  else if (runOnTbb)
  {
    std::cout << "adding a tbb worker" << std::endl;
    this->Workers.emplace_back(std::bind(process_partition_tbb, std::ref(this->Queue)));
  }
}

//-----------------------------------------------------------------------------
VISKORES_CONT MultiDeviceGradient::~MultiDeviceGradient()
{
  this->Queue.shutdown();

  //shutdown all workers
  for (auto&& thread : this->Workers)
  {
    thread.join();
  }
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::PartitionedDataSet MultiDeviceGradient::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& pds)
{
  //Step 1. Say that we have no more to submit for this PartitionedDataSet
  //This is needed to happen for each execute as we want to support
  //the same filter being used for multiple inputs
  this->Queue.reset();

  //Step 2. Construct the PartitionedDataSet we are going to fill. The size
  //signature to PartitionedDataSet just reserves size
  viskores::cont::PartitionedDataSet output;
  output.AppendPartitions(
    std::vector<viskores::cont::DataSet>(static_cast<size_t>(pds.GetNumberOfPartitions())));
  viskores::cont::PartitionedDataSet* outPtr = &output;


  //Step 3. Construct the filter we want to run on each partition
  viskores::filter::vector_analysis::Gradient gradient;
  gradient.SetComputePointGradient(this->GetComputePointGradient());
  gradient.SetActiveField(this->GetActiveFieldName());

  //Step 3b. Post 1 partition up as work and block until it is
  //complete. This is needed as currently constructing the virtual
  //Point Coordinates is not thread safe.
  auto partition = pds.cbegin();
  {
    viskores::cont::DataSet input = *partition;
    this->Queue.push( //build a lambda that is the work to do
      [=]() {
        viskores::filter::vector_analysis::Gradient perThreadGrad = gradient;

        viskores::cont::DataSet result = perThreadGrad.Execute(input);
        outPtr->ReplacePartition(0, result);
      });
    this->Queue.waitForAllTasksToComplete();
    partition++;
  }

  viskores::Id index = 1;
  for (; partition != pds.cend(); ++partition)
  {
    viskores::cont::DataSet input = *partition;
    //Step 4. For each input partition construct a lambda
    //and add it to the queue for workers to take. This
    //will allows us to have multiple works execute in a non
    //blocking manner
    this->Queue.push( //build a lambda that is the work to do
      [=]() {
        viskores::filter::vector_analysis::Gradient perThreadGrad = gradient;

        viskores::cont::DataSet result = perThreadGrad.Execute(input);
        outPtr->ReplacePartition(index, result);
      });
    index++;
  }

  // Step 5. Wait on all workers to finish
  this->Queue.waitForAllTasksToComplete();

  return output;
}

VISKORES_CONT viskores::cont::DataSet MultiDeviceGradient::DoExecute(const viskores::cont::DataSet& inData)
{
  viskores::cont::PartitionedDataSet outData = this->Execute(viskores::cont::PartitionedDataSet(inData));
  VISKORES_ASSERT(outData.GetNumberOfPartitions() == 1);
  return outData.GetPartition(0);
}
