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
#include "IOGenerator.h"

#include <viskores/Math.h>

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <chrono>
#include <random>

struct WaveField : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 3>& input, viskores::Vec<T, 3>& output) const
  {
    output[0] = input[0];
    output[1] = 0.25f * viskores::Sin(input[0]) * viskores::Cos(input[2]);
    output[2] = input[2];
  }
};

viskores::cont::DataSet make_test3DImageData(viskores::Id3 dims)
{
  using Builder = viskores::cont::DataSetBuilderUniform;
  viskores::cont::DataSet ds = Builder::Create(dims);

  viskores::cont::ArrayHandle<viskores::Vec3f> field;
  viskores::cont::Invoker invoke;
  invoke(WaveField{}, ds.GetCoordinateSystem().GetDataAsMultiplexer(), field);

  ds.AddPointField("vec_field", field);
  return ds;
}

//=================================================================
void io_generator(TaskQueue<viskores::cont::PartitionedDataSet>& queue, std::size_t numberOfTasks)
{
  //Step 1. We want to build an initial set of partitions
  //that vary in size. This way we can generate uneven
  //work to show off the viskores filter work distribution
  viskores::Id3 small(128, 128, 128);
  viskores::Id3 medium(256, 256, 128);
  viskores::Id3 large(512, 256, 128);

  std::vector<viskores::Id3> partition_sizes;
  partition_sizes.push_back(small);
  partition_sizes.push_back(medium);
  partition_sizes.push_back(large);


  std::mt19937 rng;
  //uniform_int_distribution is a closed interval [] so both the min and max
  //can be chosen values
  std::uniform_int_distribution<viskores::Id> partitionNumGen(6, 32);
  std::uniform_int_distribution<std::size_t> partitionPicker(0, partition_sizes.size() - 1);
  for (std::size_t i = 0; i < numberOfTasks; ++i)
  {
    //Step 2. Construct a random number of partitions
    const viskores::Id numberOfPartitions = partitionNumGen(rng);

    //Step 3. Randomly pick the partitions in the dataset
    viskores::cont::PartitionedDataSet pds(numberOfPartitions);
    for (viskores::Id p = 0; p < numberOfPartitions; ++p)
    {
      const auto& dims = partition_sizes[partitionPicker(rng)];
      auto partition = make_test3DImageData(dims);
      pds.AppendPartition(partition);
    }

    std::cout << "adding partitioned dataset with " << pds.GetNumberOfPartitions() << " partitions"
              << std::endl;

    //Step 4. Add the partitioned dataset to the queue. We explicitly
    //use std::move to signal that this thread can't use the
    //pds object after this call
    queue.push(std::move(pds));

    //Step 5. Go to sleep for a period of time to replicate
    //data stream in
    // std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  //Step 6. Tell the queue that we are done submitting work
  queue.shutdown();
  std::cout << "io_generator finished" << std::endl;
}
