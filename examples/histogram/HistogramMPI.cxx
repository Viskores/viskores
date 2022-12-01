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

#include <vtkm/filter/density_estimate/worklet/FieldHistogram.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>

#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>

#include <mpi.h>

namespace example
{
namespace detail
{

class DistributedHistogram
{
  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> LocalBlocks;

public:
  DistributedHistogram(vtkm::Id numLocalBlocks)
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

  vtkm::cont::ArrayHandle<vtkm::Id> ReduceAll(const vtkm::Id numBins) const
  {
    const vtkm::Id numLocalBlocks = static_cast<vtkm::Id>(this->LocalBlocks.size());
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1 && numLocalBlocks <= 1)
    {
      // no reduction necessary.
      return numLocalBlocks == 0 ? vtkm::cont::ArrayHandle<vtkm::Id>() : this->LocalBlocks[0];
    }


    // reduce local bins first.
    vtkm::cont::ArrayHandle<vtkm::Id> local;
    local.Allocate(numBins);
    std::fill(vtkm::cont::ArrayPortalToIteratorBegin(local.WritePortal()),
              vtkm::cont::ArrayPortalToIteratorEnd(local.WritePortal()),
              static_cast<vtkm::Id>(0));
    for (const auto& lbins : this->LocalBlocks)
    {
      vtkm::cont::Algorithm::Transform(local, lbins, local, vtkm::Add());
    }

    // now reduce across ranks using MPI.

    // converting to std::vector
    std::vector<vtkm::Id> send_buf(static_cast<std::size_t>(numBins));
    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(local.ReadPortal()),
              vtkm::cont::ArrayPortalToIteratorEnd(local.ReadPortal()),
              send_buf.begin());

    std::vector<vtkm::Id> recv_buf(static_cast<std::size_t>(numBins));
    MPI_Reduce(&send_buf[0],
               &recv_buf[0],
               static_cast<int>(numBins),
               sizeof(vtkm::Id) == 4 ? MPI_INT : MPI_LONG,
               MPI_SUM,
               0,
               vtkmdiy::mpi::mpi_cast(comm.handle()));

    if (comm.rank() == 0)
    {
      local.Allocate(numBins);
      std::copy(recv_buf.begin(),
                recv_buf.end(),
                vtkm::cont::ArrayPortalToIteratorBegin(local.WritePortal()));
      return local;
    }
    return vtkm::cont::ArrayHandle<vtkm::Id>();
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet HistogramMPI::DoExecute(const vtkm::cont::DataSet& input)
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
    { this->GetOutputFieldName(), vtkm::cont::Field::Association::WholeDataSet, binArray });

  // The output is a "summary" of the input, no need to map fields
  return output;
}

VTKM_CONT vtkm::cont::PartitionedDataSet HistogramMPI::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  this->PreExecute(input);
  auto result = this->Filter::DoExecutePartitions(input);
  this->PostExecute(input, result);
  return result;
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void HistogramMPI::PreExecute(const vtkm::cont::PartitionedDataSet& input)
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
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void HistogramMPI::PostExecute(const vtkm::cont::PartitionedDataSet&,
                                                vtkm::cont::PartitionedDataSet& result)
{
  // iterate and compute HistogramMPI for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfPartitions());
  for (vtkm::Id cc = 0; cc < result.GetNumberOfPartitions(); ++cc)
  {
    auto& ablock = result.GetPartition(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  vtkm::cont::DataSet output;
  vtkm::cont::Field rfield(this->GetOutputFieldName(),
                           vtkm::cont::Field::Association::WholeDataSet,
                           helper.ReduceAll(this->NumberOfBins));
  output.AddField(rfield);

  result = vtkm::cont::PartitionedDataSet(output);
}
} // namespace example
