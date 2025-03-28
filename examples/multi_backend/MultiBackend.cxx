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

#include <iostream>
#include <thread>

#include <viskores/cont/Initialize.h>
#include <viskores/cont/PartitionedDataSet.h>

#include "IOGenerator.h"
#include "MultiDeviceGradient.h"
#include "TaskQueue.h"

//This demo shows off using viskores in multiple threads in two different ways.
//
//At a high level we have 2 primary threads, an IO thread and a Worker thread
//The IO thread will generate all data using the viskores serial device, and
//will post this data to a worker queue as a viskores partitioned dataset.
//The Worker thread will pull down these viskores data and run a
//viskores filter on the partitions.
//The viskores filter it runs will itself have a worker pool which it will
//distribute work too. The number of workers is based on what device adapters
//are enabled but uses the following logic:
// -  If TBB is enabled construct a single TBB worker
// -  If CUDA is enabled construct 4 workers for each GPU on the machine
//
//Unfortunately due to some thread unsafe logic in Viskores it is currently not
//possible to have CUDA and TBB workers at the same time. So the class will
//choose CUDA over TBB when possible.
//Once the thread unsafe logic is fixed a machine that has a single CPU
//and single GPU we should expect that we will have 2 primary 'main loop'
//threads, and 5 threads for heavy 'task' work.

void partition_processing(TaskQueue<viskores::cont::PartitionedDataSet>& queue);
int main(int argc, char** argv)
{
  auto opts =
    viskores::cont::InitializeOptions::DefaultAnyDevice | viskores::cont::InitializeOptions::Strict;
  viskores::cont::Initialize(argc, argv, opts);

  //Step 1. Construct the two primary 'main loops'. The threads
  //share a queue object so we need to explicitly pass it
  //by reference (the std::ref call)
  TaskQueue<viskores::cont::PartitionedDataSet> queue;
  std::thread io(io_generator, std::ref(queue), 6);
  std::thread worker(partition_processing, std::ref(queue));

  //Step N. Wait for the work to finish
  io.join();
  worker.join();
  return 0;
}

//=================================================================
void partition_processing(TaskQueue<viskores::cont::PartitionedDataSet>& queue)
{
  //Step 1. Construct the gradient filter outside the work loop
  //so that we can reuse the thread pool it constructs
  MultiDeviceGradient gradient;
  gradient.SetComputePointGradient(true);
  while (queue.hasTasks())
  {
    //Step 2. grab the next partition skipping any that are empty
    //as empty ones can be returned when the queue is about
    //to say it has no work
    viskores::cont::PartitionedDataSet pds = queue.pop();
    if (pds.GetNumberOfPartitions() == 0)
    {
      continue;
    }

    //Step 3. Get the first field name from the partition
    std::string fieldName = pds.GetPartition(0).GetField(0).GetName();

    //Step 4. Run a multi device gradient
    gradient.SetActiveField(fieldName);
    viskores::cont::PartitionedDataSet result = gradient.Execute(pds);
    std::cout << "finished processing a partitioned dataset" << std::endl;

    //Step 5. Verify each partition has a "Gradients" field
    for (auto&& partition : result)
    {
      // std::cout << std::endl << std::endl << std::endl;
      // std::cout << "partition: " << std::endl;
      // partition.PrintSummary(std::cout);
      if (!partition.HasField("Gradients", viskores::cont::Field::Association::Points))
      {
        std::cerr << "Gradient filter failed!" << std::endl;
        std::cerr << "Missing Gradient field on output partition." << std::endl;
        break;
      }
    }
  }

  std::cout << "partition_processing finished" << std::endl;
}
