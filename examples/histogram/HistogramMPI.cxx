//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "HistogramMPI.h"

#include <viskores/filter/density_estimate/worklet/FieldHistogram.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/AssignerPartitionedDataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/FieldRangeGlobalCompute.h>

#include <viskores/thirdparty/diy/diy.h>
#include <viskores/thirdparty/diy/mpi-cast.h>

#include <mpi.h>

namespace example
{
namespace detail
{

class DistributedHistogram
{
  std::vector<viskores::cont::ArrayHandle<viskores::Id>> LocalBlocks;

public:
  DistributedHistogram(viskores::Id numLocalBlocks)
    : LocalBlocks(static_cast<size_t>(numLocalBlocks))
  {
  }

  void SetLocalHistogram(viskores::Id index, const viskores::cont::ArrayHandle<viskores::Id>& bins)
  {
    this->LocalBlocks[static_cast<size_t>(index)] = bins;
  }

  void SetLocalHistogram(viskores::Id index, const viskores::cont::Field& field)
  {
    this->SetLocalHistogram(index,
                            field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>());
  }

  viskores::cont::ArrayHandle<viskores::Id> ReduceAll(const viskores::Id numBins) const
  {
    const viskores::Id numLocalBlocks = static_cast<viskores::Id>(this->LocalBlocks.size());
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1 && numLocalBlocks <= 1)
    {
      // no reduction necessary.
      return numLocalBlocks == 0 ? viskores::cont::ArrayHandle<viskores::Id>() : this->LocalBlocks[0];
    }


    // reduce local bins first.
    viskores::cont::ArrayHandle<viskores::Id> local;
    local.Allocate(numBins);
    std::fill(viskores::cont::ArrayPortalToIteratorBegin(local.WritePortal()),
              viskores::cont::ArrayPortalToIteratorEnd(local.WritePortal()),
              static_cast<viskores::Id>(0));
    for (const auto& lbins : this->LocalBlocks)
    {
      viskores::cont::Algorithm::Transform(local, lbins, local, viskores::Add());
    }

    // now reduce across ranks using MPI.

    // converting to std::vector
    std::vector<viskores::Id> send_buf(static_cast<std::size_t>(numBins));
    std::copy(viskores::cont::ArrayPortalToIteratorBegin(local.ReadPortal()),
              viskores::cont::ArrayPortalToIteratorEnd(local.ReadPortal()),
              send_buf.begin());

    std::vector<viskores::Id> recv_buf(static_cast<std::size_t>(numBins));
    MPI_Reduce(&send_buf[0],
               &recv_buf[0],
               static_cast<int>(numBins),
               sizeof(viskores::Id) == 4 ? MPI_INT : MPI_LONG,
               MPI_SUM,
               0,
               viskoresdiy::mpi::mpi_cast(comm.handle()));

    if (comm.rank() == 0)
    {
      local.Allocate(numBins);
      std::copy(recv_buf.begin(),
                recv_buf.end(),
                viskores::cont::ArrayPortalToIteratorBegin(local.WritePortal()));
      return local;
    }
    return viskores::cont::ArrayHandle<viskores::Id>();
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet HistogramMPI::DoExecute(const viskores::cont::DataSet& input)
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
      auto handle = viskores::cont::FieldRangeGlobalCompute(
        input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
      if (handle.GetNumberOfValues() != 1)
      {
        throw viskores::cont::ErrorFilterExecution("expecting scalar field.");
      }
      this->ComputedRange = handle.ReadPortal().Get(0);
    }
  }

  viskores::cont::ArrayHandle<viskores::Id> binArray;

  auto resolveType = [&](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    T delta;

    viskores::worklet::FieldHistogram worklet;
    worklet.Run(concrete,
                this->NumberOfBins,
                static_cast<T>(this->ComputedRange.Min),
                static_cast<T>(this->ComputedRange.Max),
                delta,
                binArray);

    this->BinDelta = static_cast<viskores::Float64>(delta);
  };

  fieldArray
    .CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldScalar, VISKORES_DEFAULT_STORAGE_LIST>(
      resolveType);

  viskores::cont::DataSet output;
  output.AddField(
    { this->GetOutputFieldName(), viskores::cont::Field::Association::WholeDataSet, binArray });

  // The output is a "summary" of the input, no need to map fields
  return output;
}

VISKORES_CONT viskores::cont::PartitionedDataSet HistogramMPI::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  this->PreExecute(input);
  auto result = this->Filter::DoExecutePartitions(input);
  this->PostExecute(input, result);
  return result;
}

//-----------------------------------------------------------------------------
inline VISKORES_CONT void HistogramMPI::PreExecute(const viskores::cont::PartitionedDataSet& input)
{
  if (this->Range.IsNonEmpty())
  {
    this->ComputedRange = this->Range;
  }
  else
  {
    auto handle = viskores::cont::FieldRangeGlobalCompute(
      input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    if (handle.GetNumberOfValues() != 1)
    {
      throw viskores::cont::ErrorFilterExecution("expecting scalar field.");
    }
    this->ComputedRange = handle.ReadPortal().Get(0);
  }
}

//-----------------------------------------------------------------------------
inline VISKORES_CONT void HistogramMPI::PostExecute(const viskores::cont::PartitionedDataSet&,
                                                viskores::cont::PartitionedDataSet& result)
{
  // iterate and compute HistogramMPI for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfPartitions());
  for (viskores::Id cc = 0; cc < result.GetNumberOfPartitions(); ++cc)
  {
    auto& ablock = result.GetPartition(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  viskores::cont::DataSet output;
  viskores::cont::Field rfield(this->GetOutputFieldName(),
                           viskores::cont::Field::Association::WholeDataSet,
                           helper.ReduceAll(this->NumberOfBins));
  output.AddField(rfield);

  result = viskores::cont::PartitionedDataSet(output);
}
} // namespace example
