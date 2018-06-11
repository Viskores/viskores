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
#include "IOGenerator.h"

#include <vtkm/Math.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <chrono>
#include <random>

struct WaveField : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn<Vec3>, FieldOut<Vec3>);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& input, vtkm::Vec<T, 3>& output) const
  {
    output[0] = input[0];
    output[1] = 0.25f * vtkm::Sin(input[0]) * vtkm::Cos(input[2]);
    output[2] = input[2];
  }
};

vtkm::cont::DataSet make_test3DImageData(int xdim, int ydim, int zdim)
{
  using Builder = vtkm::cont::DataSetBuilderUniform;
  using FieldAdd = vtkm::cont::DataSetFieldAdd;
  vtkm::cont::DataSet ds = Builder::Create(vtkm::Id3{ xdim, ydim, zdim });

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> field;
  vtkm::worklet::DispatcherMapField<WaveField, vtkm::cont::DeviceAdapterTagSerial> dispatcher;
  dispatcher.Invoke(ds.GetCoordinateSystem(), field);

  FieldAdd::AddPointField(ds, "vec_field", field);
  return ds;
}

//=================================================================
void io_generator(TaskQueue<vtkm::cont::MultiBlock>& queue, std::size_t numberOfTasks)
{
  //Step 1. We want to build an initial set of blocks
  //that vary in size. This way we can generate uneven
  //work to show off the vtk-m filter work distribution
  vtkm::cont::DataSet small = make_test3DImageData(128, 128, 128);
  vtkm::cont::DataSet medium = make_test3DImageData(256, 256, 128);
  vtkm::cont::DataSet large = make_test3DImageData(512, 512, 128);

  std::vector<vtkm::cont::DataSet> blocks;
  blocks.push_back(small);
  blocks.push_back(medium);
  blocks.push_back(large);


  std::mt19937 rng;
  //uniform_int_distribution is a closed interval [] so both the min and max
  //can be chosen values
  std::uniform_int_distribution<vtkm::Id> blockNumGen(6, 32);
  std::uniform_int_distribution<std::size_t> blockPicker(0, blocks.size() - 1);
  for (std::size_t i = 0; i < numberOfTasks; ++i)
  {
    //Step 2. Construct a random number of blocks
    const vtkm::Id numberOfBlocks = blockNumGen(rng);

    //Step 3. Randomly pick the blocks in the dataset
    vtkm::cont::MultiBlock mb(numberOfBlocks);
    for (vtkm::Id b = 0; b < numberOfBlocks; ++b)
    {
      mb.AddBlock(blocks[blockPicker(rng)]);
    }

    std::cout << "adding multi-block with " << mb.GetNumberOfBlocks() << " blocks" << std::endl;

    //Step 4. Add the multi-block to the queue. We explicitly
    //use std::move to signal that this thread can't use the
    //mb object after this call
    queue.push(std::move(mb));

    //Step 5. Go to sleep for a period of time to replicate
    //data stream in
    // std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  //Step 6. Tell the queue that we are done submitting work
  queue.shutdown();
  std::cout << "io_generator finished" << std::endl;
}
