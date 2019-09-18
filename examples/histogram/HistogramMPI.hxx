//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/FieldHistogram.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>

#include <vtkm/thirdparty/diy/diy.h>

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
    this->SetLocalHistogram(index, field.GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Id>>());
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
    std::fill(vtkm::cont::ArrayPortalToIteratorBegin(local.GetPortalControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(local.GetPortalControl()),
              static_cast<vtkm::Id>(0));
    for (const auto& lbins : this->LocalBlocks)
    {
      vtkm::cont::Algorithm::Transform(local, lbins, local, vtkm::Add());
    }

    // now reduce across ranks using MPI.

    // converting to std::vector
    std::vector<vtkm::Id> send_buf(static_cast<std::size_t>(numBins));
    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(local.GetPortalConstControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(local.GetPortalConstControl()),
              send_buf.begin());

    std::vector<vtkm::Id> recv_buf(static_cast<std::size_t>(numBins));
    MPI_Reduce(&send_buf[0],
               &recv_buf[0],
               static_cast<int>(numBins),
               sizeof(vtkm::Id) == 4 ? MPI_INT : MPI_LONG,
               MPI_SUM,
               0,
               comm);

    if (comm.rank() == 0)
    {
      local.Allocate(numBins);
      std::copy(recv_buf.begin(),
                recv_buf.end(),
                vtkm::cont::ArrayPortalToIteratorBegin(local.GetPortalControl()));
      return local;
    }
    return vtkm::cont::ArrayHandle<vtkm::Id>();
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
inline VTKM_CONT HistogramMPI::HistogramMPI()
  : NumberOfBins(10)
  , BinDelta(0)
  , ComputedRange()
  , Range()
{
  this->SetOutputFieldName("histogram");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet HistogramMPI::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::ArrayHandle<vtkm::Id> binArray;
  T delta;

  vtkm::worklet::FieldHistogram worklet;
  if (this->ComputedRange.IsNonEmpty())
  {
    worklet.Run(field,
                this->NumberOfBins,
                static_cast<T>(this->ComputedRange.Min),
                static_cast<T>(this->ComputedRange.Max),
                delta,
                binArray);
  }
  else
  {
    worklet.Run(field, this->NumberOfBins, this->ComputedRange, delta, binArray);
  }

  this->BinDelta = static_cast<vtkm::Float64>(delta);
  vtkm::cont::DataSet output;
  vtkm::cont::Field rfield(
    this->GetOutputFieldName(), vtkm::cont::Field::Association::WHOLE_MESH, binArray);
  output.AddField(rfield);
  return output;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void HistogramMPI::PreExecute(const vtkm::cont::PartitionedDataSet& input,
                                               const vtkm::filter::PolicyBase<DerivedPolicy>&)
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
    this->ComputedRange = handle.GetPortalConstControl().Get(0);
  }
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void HistogramMPI::PostExecute(const vtkm::cont::PartitionedDataSet&,
                                                vtkm::cont::PartitionedDataSet& result,
                                                const vtkm::filter::PolicyBase<DerivedPolicy>&)
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
                           vtkm::cont::Field::Association::WHOLE_MESH,
                           helper.ReduceAll(this->NumberOfBins));
  output.AddField(rfield);

  result = vtkm::cont::PartitionedDataSet(output);
}
} // namespace example
