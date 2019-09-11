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

#include <vtkm/Math.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <chrono>
#include <random>

struct WaveField : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& input, vtkm::Vec<T, 3>& output) const
  {
    output[0] = input[0];
    output[1] = 0.25f * vtkm::Sin(input[0]) * vtkm::Cos(input[2]);
    output[2] = input[2];
  }
};

vtkm::cont::DataSet make_test3DImageData(vtkm::Id3 dims)
{
  using Builder = vtkm::cont::DataSetBuilderUniform;
  using FieldAdd = vtkm::cont::DataSetFieldAdd;
  vtkm::cont::DataSet ds = Builder::Create(dims);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> field;
  vtkm::cont::Invoker invoke;
  invoke(WaveField{}, ds.GetCoordinateSystem(), field);

  FieldAdd::AddPointField(ds, "vec_field", field);
  return ds;
}

//=================================================================
void io_generator(TaskQueue<vtkm::cont::PartitionedDataSet>& queue, std::size_t numberOfTasks)
{
  //Step 1. We want to build an initial set of partitions
  //that vary in size. This way we can generate uneven
  //work to show off the vtk-m filter work distribution
  vtkm::Id3 small(128, 128, 128);
  vtkm::Id3 medium(256, 256, 128);
  vtkm::Id3 large(512, 256, 128);

  std::vector<vtkm::Id3> partition_sizes;
  partition_sizes.push_back(small);
  partition_sizes.push_back(medium);
  partition_sizes.push_back(large);


  std::mt19937 rng;
  //uniform_int_distribution is a closed interval [] so both the min and max
  //can be chosen values
  std::uniform_int_distribution<vtkm::Id> partitionNumGen(6, 32);
  std::uniform_int_distribution<std::size_t> partitionPicker(0, partition_sizes.size() - 1);
  for (std::size_t i = 0; i < numberOfTasks; ++i)
  {
    //Step 2. Construct a random number of partitions
    const vtkm::Id numberOfPartitions = partitionNumGen(rng);

    //Step 3. Randomly pick the partitions in the dataset
    vtkm::cont::PartitionedDataSet pds(numberOfPartitions);
    for (vtkm::Id p = 0; p < numberOfPartitions; ++p)
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
