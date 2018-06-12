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

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/FieldHistogram.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/AssignerMultiBlock.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/FieldRangeGlobalCompute.h>
#include <vtkm/cont/diy/Serialization.h>
#include <vtkm/filter/internal/CreateResult.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include VTKM_DIY(diy/decomposition.hpp)
#include VTKM_DIY(diy/master.hpp)
#include VTKM_DIY(diy/partners/broadcast.hpp)
#include VTKM_DIY(diy/partners/swap.hpp)
#include VTKM_DIY(diy/reduce.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace vtkm
{
namespace filter
{
namespace detail
{
class DistributedHistogram
{
  class Reducer
  {
  public:
    void operator()(vtkm::cont::ArrayHandle<vtkm::Id>* result,
                    const diy::ReduceProxy& srp,
                    const diy::RegularMergePartners&) const
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

    diy::Master master(
      comm,
      /*threads*/ 1,
      /*limit*/ -1,
      []() -> void* { return new vtkm::cont::ArrayHandle<vtkm::Id>(); },
      [](void* ptr) { delete static_cast<vtkm::cont::ArrayHandle<vtkm::Id>*>(ptr); });

    vtkm::cont::AssignerMultiBlock assigner(numLocalBlocks);
    diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
      /*dims*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);

    assert(static_cast<vtkm::Id>(master.size()) == numLocalBlocks);
    for (vtkm::Id cc = 0; cc < numLocalBlocks; ++cc)
    {
      *master.block<ArrayType>(static_cast<int>(cc)) = this->LocalBlocks[static_cast<size_t>(cc)];
    }

    diy::RegularMergePartners partners(decomposer, /*k=*/2);
    // reduce to block-0.
    diy::reduce(master, assigner, partners, Reducer());

    ArrayType result;
    if (master.local(0))
    {
      result = *master.block<ArrayType>(master.lid(0));
    }

    this->Broadcast(result);
    return result;
  }

private:
  void Broadcast(vtkm::cont::ArrayHandle<vtkm::Id>& data) const
  {
    // broadcast to all ranks (and not blocks).
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() > 1)
    {
      using ArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
      diy::Master master(
        comm,
        /*threads*/ 1,
        /*limit*/ -1,
        []() -> void* { return new vtkm::cont::ArrayHandle<vtkm::Id>(); },
        [](void* ptr) { delete static_cast<vtkm::cont::ArrayHandle<vtkm::Id>*>(ptr); });

      diy::ContiguousAssigner assigner(comm.size(), comm.size());
      diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
        1, diy::interval(0, comm.size() - 1), comm.size());
      decomposer.decompose(comm.rank(), assigner, master);
      assert(master.size() == 1); // number of local blocks should be 1 per rank.
      *master.block<ArrayType>(0) = data;
      diy::RegularBroadcastPartners partners(decomposer, /*k=*/2);
      diy::reduce(master, assigner, partners, Reducer());
      data = *master.block<ArrayType>(0);
    }
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
inline VTKM_CONT Histogram::Histogram()
  : NumberOfBins(10)
  , BinDelta(0)
  , ComputedRange()
  , Range()
{
  this->SetOutputFieldName("histogram");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet Histogram::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
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
                binArray,
                device);
  }
  else
  {
    worklet.Run(field, this->NumberOfBins, this->ComputedRange, delta, binArray, device);
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
inline VTKM_CONT void Histogram::PreExecute(const vtkm::cont::MultiBlock& input,
                                            const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using TypeList = typename DerivedPolicy::FieldTypeList;
  using StorageList = typename DerivedPolicy::FieldStorageList;
  if (this->Range.IsNonEmpty())
  {
    this->ComputedRange = this->Range;
  }
  else
  {
    auto handle = vtkm::cont::FieldRangeGlobalCompute(input,
                                                      this->GetActiveFieldName(),
                                                      this->GetActiveFieldAssociation(),
                                                      TypeList(),
                                                      StorageList());
    if (handle.GetNumberOfValues() != 1)
    {
      throw vtkm::cont::ErrorFilterExecution("expecting scalar field.");
    }
    this->ComputedRange = handle.GetPortalConstControl().Get(0);
  }
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void Histogram::PostExecute(const vtkm::cont::MultiBlock&,
                                             vtkm::cont::MultiBlock& result,
                                             const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  // iterate and compute histogram for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfBlocks());
  for (vtkm::Id cc = 0; cc < result.GetNumberOfBlocks(); ++cc)
  {
    auto& ablock = result.GetBlock(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  vtkm::cont::DataSet output;
  vtkm::cont::Field rfield(
    this->GetOutputFieldName(), vtkm::cont::Field::Association::WHOLE_MESH, helper.ReduceAll());
  output.AddField(rfield);

  result = vtkm::cont::MultiBlock(output);
}
}
} // namespace vtkm::filter
