//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/density_estimate/Histogram.h>
#include <vtkm/filter/density_estimate/worklet/FieldHistogram.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>
#include <vtkm/cont/Serialization.h>

#include <vtkm/thirdparty/diy/diy.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
namespace detail
{
class DistributedHistogram
{
  class Reducer
  {
  public:
    void operator()(vtkm::cont::ArrayHandle<vtkm::Id>* result,
                    const vtkmdiy::ReduceProxy& srp,
                    const vtkmdiy::RegularMergePartners&) const
    {
      const auto selfid = srp.gid();
      // 1. dequeue.
      std::vector<int> incoming;
      srp.incoming(incoming);
      for (const int gid : incoming)
      {
        if (gid != selfid)
        {
          vtkm::cont::ArrayHandle<vtkm::Id> in;
          srp.dequeue(gid, in);
          if (result->GetNumberOfValues() == 0)
          {
            *result = in;
          }
          else
          {
            vtkm::cont::Algorithm::Transform(*result, in, *result, vtkm::Add());
          }
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
    }
  };

  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> LocalBlocks;

public:
  explicit DistributedHistogram(vtkm::Id numLocalBlocks)
    : LocalBlocks(static_cast<size_t>(numLocalBlocks))
  {
  }

  void SetLocalHistogram(vtkm::Id index, const vtkm::cont::ArrayHandle<vtkm::Id>& bins)
  {
    this->LocalBlocks[static_cast<size_t>(index)] = bins;
  }

  void SetLocalHistogram(vtkm::Id index, const vtkm::cont::Field& field)
  {
    this->SetLocalHistogram(index,
                            field.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>());
  }

  vtkm::cont::ArrayHandle<vtkm::Id> ReduceAll() const
  {
    using ArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;

    const vtkm::Id numLocalBlocks = static_cast<vtkm::Id>(this->LocalBlocks.size());
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1 && numLocalBlocks <= 1)
    {
      // no reduction necessary.
      return numLocalBlocks == 0 ? ArrayType() : this->LocalBlocks[0];
    }

    vtkmdiy::Master master(
      comm,
      /*threads*/ 1,
      /*limit*/ -1,
      []() -> void* { return new vtkm::cont::ArrayHandle<vtkm::Id>(); },
      [](void* ptr) { delete static_cast<vtkm::cont::ArrayHandle<vtkm::Id>*>(ptr); });

    vtkm::cont::AssignerPartitionedDataSet assigner(numLocalBlocks);
    vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds> decomposer(
      /*dims*/ 1, vtkmdiy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);

    assert(static_cast<vtkm::Id>(master.size()) == numLocalBlocks);
    for (vtkm::Id cc = 0; cc < numLocalBlocks; ++cc)
    {
      *master.block<ArrayType>(static_cast<int>(cc)) = this->LocalBlocks[static_cast<size_t>(cc)];
    }

    vtkmdiy::RegularMergePartners partners(decomposer, /*k=*/2);
    // reduce to block-0.
    vtkmdiy::reduce(master, assigner, partners, Reducer());

    ArrayType result;
    if (master.local(0))
    {
      result = *master.block<ArrayType>(master.lid(0));
    }

    this->Broadcast(result);
    return result;
  }

private:
  static void Broadcast(vtkm::cont::ArrayHandle<vtkm::Id>& data)
  {
    // broadcast to all ranks (and not blocks).
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() > 1)
    {
      using ArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
      vtkmdiy::Master master(
        comm,
        /*threads*/ 1,
        /*limit*/ -1,
        []() -> void* { return new vtkm::cont::ArrayHandle<vtkm::Id>(); },
        [](void* ptr) { delete static_cast<vtkm::cont::ArrayHandle<vtkm::Id>*>(ptr); });

      vtkmdiy::ContiguousAssigner assigner(comm.size(), comm.size());
      vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds> decomposer(
        1, vtkmdiy::interval(0, comm.size() - 1), comm.size());
      decomposer.decompose(comm.rank(), assigner, master);
      assert(master.size() == 1); // number of local blocks should be 1 per rank.
      *master.block<ArrayType>(0) = data;
      vtkmdiy::RegularBroadcastPartners partners(decomposer, /*k=*/2);
      vtkmdiy::reduce(master, assigner, partners, Reducer());
      data = *master.block<ArrayType>(0);
    }
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
VTKM_CONT Histogram::Histogram()
{
  this->SetOutputFieldName("histogram");
}

VTKM_CONT vtkm::cont::DataSet Histogram::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& fieldArray = this->GetFieldFromDataSet(input).GetData();

  if (!this->InExecutePartitions)
  {
    // Handle initialization that would be done in PreExecute if the data set had partitions.
    if (this->Range.IsNonEmpty())
    {
      this->ComputedRange = this->Range;
    }
    else
    {
      auto handle = vtkm::cont::FieldRangeGlobalCompute(
        input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
      if (handle.GetNumberOfValues() != 1)
      {
        throw vtkm::cont::ErrorFilterExecution("expecting scalar field.");
      }
      this->ComputedRange = handle.ReadPortal().Get(0);
    }
  }

  vtkm::cont::ArrayHandle<vtkm::Id> binArray;

  auto resolveType = [&](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    T delta;

    vtkm::worklet::FieldHistogram worklet;
    worklet.Run(concrete,
                this->NumberOfBins,
                static_cast<T>(this->ComputedRange.Min),
                static_cast<T>(this->ComputedRange.Max),
                delta,
                binArray);

    this->BinDelta = static_cast<vtkm::Float64>(delta);
  };

  fieldArray
    .CastAndCallForTypesWithFloatFallback<vtkm::TypeListFieldScalar, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType);

  vtkm::cont::DataSet output;
  output.AddField(
    { this->GetOutputFieldName(), vtkm::cont::Field::Association::WholeMesh, binArray });

  // The output is a "summary" of the input, no need to map fields
  return output;
}

VTKM_CONT vtkm::cont::PartitionedDataSet Histogram::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  this->PreExecute(input);
  auto result = this->NewFilter::DoExecutePartitions(input);
  this->PostExecute(input, result);
  return result;
}

//-----------------------------------------------------------------------------
VTKM_CONT void Histogram::PreExecute(const vtkm::cont::PartitionedDataSet& input)
{
  if (this->Range.IsNonEmpty())
  {
    this->ComputedRange = this->Range;
  }
  else
  {
    auto handle = vtkm::cont::FieldRangeGlobalCompute(
      input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    if (handle.GetNumberOfValues() != 1)
    {
      throw vtkm::cont::ErrorFilterExecution("expecting scalar field.");
    }
    this->ComputedRange = handle.ReadPortal().Get(0);
  }
  this->InExecutePartitions = true;
}

//-----------------------------------------------------------------------------
VTKM_CONT void Histogram::PostExecute(const vtkm::cont::PartitionedDataSet&,
                                      vtkm::cont::PartitionedDataSet& result)
{
  this->InExecutePartitions = false;
  // iterate and compute histogram for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfPartitions());
  for (vtkm::Id cc = 0; cc < result.GetNumberOfPartitions(); ++cc)
  {
    auto& ablock = result.GetPartition(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  vtkm::cont::DataSet output;
  vtkm::cont::Field rfield(
    this->GetOutputFieldName(), vtkm::cont::Field::Association::WholeMesh, helper.ReduceAll());
  output.AddField(rfield);

  result = vtkm::cont::PartitionedDataSet(output);
}
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
