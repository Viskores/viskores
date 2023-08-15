//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/density_estimate/Statistics.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/worklet/DescriptiveStatistics.h>
#ifdef VTKM_ENABLE_MPI
#include <mpi.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>
#endif

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
//refer to this paper https://www.osti.gov/servlets/purl/1028931
//for the math of computing distributed statistics
//using anonymous namespace
namespace
{
using StatValueType = vtkm::worklet::DescriptiveStatistics::StatState<vtkm::FloatDefault>;
class DistributedStatistics
{
  vtkm::cont::ArrayHandle<StatValueType> localStatisticsValues;

public:
  DistributedStatistics(vtkm::Id numLocalBlocks)
  {
    this->localStatisticsValues.Allocate(numLocalBlocks);
  }

  void SetLocalStatistics(vtkm::Id index, StatValueType& value)
  {
    this->localStatisticsValues.WritePortal().Set(index, value);
  }

  StatValueType ReduceStatisticsDiy() const
  {
    using Algorithm = vtkm::cont::Algorithm;
    // The StatValueType struct overloads the + operator. Reduce is using to properly
    // combine statistical measures such as mean, standard deviation, and others. So,
    // the Reduce is computing the global statistics over partitions rather than a
    // simple sum.
    StatValueType statePerRank = Algorithm::Reduce(this->localStatisticsValues, StatValueType{});
    StatValueType stateResult = statePerRank;
#ifdef VTKM_ENABLE_MPI
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1)
    {
      return statePerRank;
    }

    vtkmdiy::Master master(
      comm,
      1,
      -1,
      []() -> void* { return new StatValueType(); },
      [](void* ptr) { delete static_cast<StatValueType*>(ptr); });

    vtkmdiy::ContiguousAssigner assigner(/*num ranks*/ comm.size(),
                                         /*global-num-blocks*/ comm.size());
    vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds> decomposer(
      /*dims*/ 1, vtkmdiy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);
    VTKM_ASSERT(static_cast<vtkm::Id>(master.size()) == 1);

    //adding data into master
    *master.block<StatValueType>(0) = statePerRank;
    auto callback = [](StatValueType* result,
                       const vtkmdiy::ReduceProxy& srp,
                       const vtkmdiy::RegularMergePartners&) {
      const auto selfid = srp.gid();
      // 1. dequeue.
      std::vector<int> incoming;
      srp.incoming(incoming);
      for (const int gid : incoming)
      {
        if (gid != selfid)
        {
          StatValueType inData;
          srp.dequeue(gid, inData);
          *result = *result + inData;
        }
      }
      // 2. enqueue
      for (int cc = 0; cc < srp.out_link().size(); ++cc)
      {
        auto target = srp.out_link().target(cc);
        if (target.gid != selfid)
        {
          srp.enqueue(target, *result);
        }
      }
    };

    vtkmdiy::RegularMergePartners partners(decomposer, /*k=*/2);
    vtkmdiy::reduce(master, assigner, partners, callback);

    //only rank 0 process returns the correct results
    if (master.local(0))
    {
      stateResult = *master.block<StatValueType>(0);
    }
    else
    {
      stateResult = StatValueType();
    }
#endif
    return stateResult;
  }
};
}

vtkm::FloatDefault ExtractVariable(vtkm::cont::DataSet dataset, const std::string& statName)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array;
  dataset.GetField(statName).GetData().AsArrayHandle(array);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType portal = array.ReadPortal();
  vtkm::FloatDefault value = portal.Get(0);
  return value;
}

template <typename T>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::FloatDefault> SaveDataIntoArray(const T value)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> stat;
  stat.Allocate(1);
  stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(value));
  return stat;
}

VTKM_CONT StatValueType GetStatValueFromDataSet(const vtkm::cont::DataSet& data)
{
  vtkm::FloatDefault N = ExtractVariable(data, "N");
  vtkm::FloatDefault Min = ExtractVariable(data, "Min");
  vtkm::FloatDefault Max = ExtractVariable(data, "Max");
  vtkm::FloatDefault Sum = ExtractVariable(data, "Sum");
  vtkm::FloatDefault Mean = ExtractVariable(data, "Mean");
  vtkm::FloatDefault M2 = ExtractVariable(data, "M2");
  vtkm::FloatDefault M3 = ExtractVariable(data, "M3");
  vtkm::FloatDefault M4 = ExtractVariable(data, "M4");
  return StatValueType(N, Min, Max, Sum, Mean, M2, M3, M4);
}

template <typename DataSetType>
VTKM_CONT void SaveIntoDataSet(StatValueType& statValue,
                               DataSetType& output,
                               vtkm::cont::Field::Association association)
{
  output.AddField({ "N", association, SaveDataIntoArray(statValue.N()) });
  output.AddField({ "Min", association, SaveDataIntoArray(statValue.Min()) });
  output.AddField({ "Max", association, SaveDataIntoArray(statValue.Max()) });
  output.AddField({ "Sum", association, SaveDataIntoArray(statValue.Sum()) });
  output.AddField({ "Mean", association, SaveDataIntoArray(statValue.Mean()) });
  output.AddField({ "M2", association, SaveDataIntoArray(statValue.M2()) });
  output.AddField({ "M3", association, SaveDataIntoArray(statValue.M3()) });
  output.AddField({ "M4", association, SaveDataIntoArray(statValue.M4()) });
  output.AddField({ "SampleStddev", association, SaveDataIntoArray(statValue.SampleStddev()) });
  output.AddField(
    { "PopulationStddev", association, SaveDataIntoArray(statValue.PopulationStddev()) });
  output.AddField({ "SampleVariance", association, SaveDataIntoArray(statValue.SampleVariance()) });
  output.AddField(
    { "PopulationVariance", association, SaveDataIntoArray(statValue.PopulationVariance()) });
  output.AddField({ "Skewness", association, SaveDataIntoArray(statValue.Skewness()) });
  output.AddField({ "Kurtosis", association, SaveDataIntoArray(statValue.Kurtosis()) });
}

VTKM_CONT vtkm::cont::DataSet Statistics::DoExecute(const vtkm::cont::DataSet& inData)
{
  vtkm::worklet::DescriptiveStatistics worklet;
  vtkm::cont::DataSet output;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> input;
  //TODO: GetFieldFromDataSet will throw an exception if the targeted Field does not exist in the data set
  ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(inData).GetData(), input);
  StatValueType result = worklet.Run(input);
  SaveIntoDataSet<vtkm::cont::DataSet>(
    result, output, vtkm::cont::Field::Association::WholeDataSet);
  return output;
}

VTKM_CONT vtkm::cont::PartitionedDataSet Statistics::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  // This operation will create a partitioned data set with a partition matching each input partition
  // containing the local statistics. It will iterate through each partition in the input and call the
  // DoExecute function. This is the same behavior as if we did not implement `DoExecutePartitions`.
  // It has the added benefit of optimizations for concurrently executing small blocks.
  vtkm::cont::PartitionedDataSet output = this->FilterField::DoExecutePartitions(input);
  vtkm::Id numPartitions = input.GetNumberOfPartitions();
  DistributedStatistics helper(numPartitions);
  for (vtkm::Id i = 0; i < numPartitions; ++i)
  {
    const vtkm::cont::DataSet& localDS = output.GetPartition(i);
    StatValueType localStatisticsValues = GetStatValueFromDataSet(localDS);
    helper.SetLocalStatistics(i, localStatisticsValues);
  }
  StatValueType result = helper.ReduceStatisticsDiy();
  SaveIntoDataSet<vtkm::cont::PartitionedDataSet>(
    result, output, vtkm::cont::Field::Association::Global);
  return output;
}
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
