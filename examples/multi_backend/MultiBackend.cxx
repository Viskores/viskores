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

#include <iostream>
#include <thread>

#include <vtkm/cont/MultiBlock.h>

#include "IOGenerator.h"
#include "MultiDeviceGradient.h"
#include "TaskQueue.h"

//This demo shows off using vtk-m in multiple threads in two different ways.
//
//At a high level we have 2 primary threads, an IO thread and a Worker thread
//The IO thread will generate all data using the vtk-m serial device, and
//will post this data to a worker queue as a vtk-m multiblock.
//The Worker thread will pull down these vtk-m multiblock data and run a
//vtk-m filter on the multiblock.
//The vtk-m filter it runs will itself have a worker pool which it will
//distribute work too. The number of workers is based on what device adapters
//are enabled but uses the following logic:
// -  If TBB is enabled construct a single TBB worker
// -  If CUDA is enabled construct 4 workers for each GPU on the machine
//
//Unfortunately due to some thread unsafe logic in VTK-m it is currently not
//possible to have CUDA and TBB workers at the same time. So the class will
//choose CUDA over TBB when possible.
//Once the thread unsafe logic is fixed a machine that has a single CPU
//and single GPU we should expect that we will have 2 primary 'main loop'
//threads, and 5 threads for heavy 'task' work.

void multiblock_processing(TaskQueue<vtkm::cont::MultiBlock>& queue);
int main(int, char**)
{
  //Step 1. Construct the two primary 'main loops'. The threads
  //share a queue object so we need to explicitly pass it
  //by reference (the std::ref call)
  TaskQueue<vtkm::cont::MultiBlock> queue;
  std::thread io(io_generator, std::ref(queue), 12);
  std::thread worker(multiblock_processing, std::ref(queue));

  //Step N. Wait for the work to finish
  io.join();
  worker.join();
  return 0;
}

//=================================================================
void multiblock_processing(TaskQueue<vtkm::cont::MultiBlock>& queue)
{
  //Step 1. Construct the gradient filter outside the work loop
  //so that we can reuse the thread pool it constructs
  MultiDeviceGradient gradient;
  gradient.SetComputePointGradient(true);
  while (queue.hasTasks())
  {
    //Step 2. grab the next multi-block skipping any that are empty
    //as empty ones can be returned when the queue is about
    //to say it has no work
    vtkm::cont::MultiBlock mb = queue.pop();
    if (mb.GetNumberOfBlocks() == 0)
    {
      continue;
    }

    //Step 3. Get the first field name from the multi-block
    std::string fieldName = mb.GetBlock(0).GetField(0).GetName();

    //Step 4. Run a multi device gradient
    gradient.SetActiveField(fieldName);
    vtkm::cont::MultiBlock result = gradient.Execute(mb);
    std::cout << "finished processing a multi-block" << std::endl;

    //Step 5. Verify each block has a "Gradients" field
    for (auto&& block : result)
    {
      // std::cout << std::endl << std::endl << std::endl;
      // std::cout << "block: " << std::endl;
      // block.PrintSummary(std::cout);
      try
      {
        const auto& field = block.GetField("Gradients", vtkm::cont::Field::Association::POINTS);
        (void)field;
      }
      catch (vtkm::cont::ErrorBadValue)
      {
        std::cerr << "gradient filter failed!" << std::endl;
        break;
      }
    }
  }

  std::cout << "multiblock_processing finished" << std::endl;
}
