//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/filter/Gradient.h>


namespace
{
int determine_cuda_gpu_count()
{
  int count = 0;
#if defined(VTKM_ENABLE_CUDA)
  int numberOfDevices = 0;
  auto res = cudaGetDeviceCount(&numberOfDevices);
  if (res == cudaSuccess)
  {
    count = numberOfDevices;
  }
#endif
  return count;
}

void process_partition_tbb(RuntimeTaskQueue& queue)
{
  //Step 1. Set the device adapter to this thread to TBB.
  //This makes sure that any vtkm::filters used by our
  //task operate only on TBB. The "global" thread tracker
  //is actually thread-local, so we can use that.
  //
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagTBB{});

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
  //Step 1. Set the device adapter to this thread to TBB.
  //This makes sure that any vtkm::filters used by our
  //task operate only on TBB. The "global" thread tracker
  //is actually thread-local, so we can use that.
  //
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagOpenMP{});

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

void process_partition_cuda(RuntimeTaskQueue& queue, int gpuId)
{
  //Step 1. Set the device adapter to this thread to cuda.
  //This makes sure that any vtkm::filters used by our
  //task operate only on cuda.  The "global" thread tracker
  //is actually thread-local, so we can use that.
  //
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
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
VTKM_CONT MultiDeviceGradient::MultiDeviceGradient()
  : ComputePointGradient(false)
  , Queue()
  , Workers()
{
  //Step 1. Determine the number of workers we want
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  const bool runOnCuda = tracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda{});
  const bool runOnOpenMP = tracker.CanRunOn(vtkm::cont::DeviceAdapterTagOpenMP{});
  const bool runOnTbb = tracker.CanRunOn(vtkm::cont::DeviceAdapterTagTBB{});

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
    const int gpu_count = determine_cuda_gpu_count();
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
VTKM_CONT MultiDeviceGradient::~MultiDeviceGradient()
{
  this->Queue.shutdown();

  //shutdown all workers
  for (auto&& thread : this->Workers)
  {
    thread.join();
  }
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet MultiDeviceGradient::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& pds,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  //Step 1. Say that we have no more to submit for this PartitionedDataSet
  //This is needed to happen for each execute as we want to support
  //the same filter being used for multiple inputs
  this->Queue.reset();

  //Step 2. Construct the PartitionedDataSet we are going to fill. The size
  //signature to PartitionedDataSet just reserves size
  vtkm::cont::PartitionedDataSet output;
  output.AppendPartitions(
    std::vector<vtkm::cont::DataSet>(static_cast<size_t>(pds.GetNumberOfPartitions())));
  vtkm::cont::PartitionedDataSet* outPtr = &output;


  //Step 3. Construct the filter we want to run on each partition
  vtkm::filter::Gradient gradient;
  gradient.SetComputePointGradient(this->GetComputePointGradient());
  gradient.SetActiveField(this->GetActiveFieldName());

  //Step 3b. Post 1 partition up as work and block until it is
  //complete. This is needed as currently constructing the virtual
  //Point Coordinates is not thread safe.
  auto partition = pds.cbegin();
  {
    vtkm::cont::DataSet input = *partition;
    this->Queue.push( //build a lambda that is the work to do
      [=]() {
        vtkm::filter::Gradient perThreadGrad = gradient;

        vtkm::cont::DataSet result = perThreadGrad.Execute(input, policy);
        outPtr->ReplacePartition(0, result);
      });
    this->Queue.waitForAllTasksToComplete();
    partition++;
  }

  vtkm::Id index = 1;
  for (; partition != pds.cend(); ++partition)
  {
    vtkm::cont::DataSet input = *partition;
    //Step 4. For each input partition construct a lambda
    //and add it to the queue for workers to take. This
    //will allows us to have multiple works execute in a non
    //blocking manner
    this->Queue.push( //build a lambda that is the work to do
      [=]() {
        vtkm::filter::Gradient perThreadGrad = gradient;

        vtkm::cont::DataSet result = perThreadGrad.Execute(input, policy);
        outPtr->ReplacePartition(index, result);
      });
    index++;
  }

  // Step 5. Wait on all workers to finish
  this->Queue.waitForAllTasksToComplete();

  return output;
}
