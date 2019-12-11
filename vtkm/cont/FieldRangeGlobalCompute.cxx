//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/FieldRangeGlobalCompute.h>

#include <vtkm/cont/EnvironmentTracker.h>

#include <vtkm/thirdparty/diy/diy.h>

#include <algorithm>
#include <functional>

namespace vtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(const vtkm::cont::DataSet& dataset,
                                                             const std::string& name,
                                                             vtkm::cont::Field::Association assoc)
{
  return detail::FieldRangeGlobalComputeImpl(dataset, name, assoc, VTKM_DEFAULT_TYPE_LIST());
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  vtkm::cont::Field::Association assoc)
{
  return detail::FieldRangeGlobalComputeImpl(pds, name, assoc, VTKM_DEFAULT_TYPE_LIST());
}

//-----------------------------------------------------------------------------
namespace detail
{
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> MergeRangesGlobal(
  const vtkm::cont::ArrayHandle<vtkm::Range>& ranges)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() == 1)
  {
    return ranges;
  }

  std::vector<vtkm::Range> v_ranges(static_cast<size_t>(ranges.GetNumberOfValues()));
  std::copy(vtkm::cont::ArrayPortalToIteratorBegin(ranges.GetPortalConstControl()),
            vtkm::cont::ArrayPortalToIteratorEnd(ranges.GetPortalConstControl()),
            v_ranges.begin());

  using VectorOfRangesT = std::vector<vtkm::Range>;

  vtkmdiy::Master master(comm,
                         1,
                         -1,
                         []() -> void* { return new VectorOfRangesT(); },
                         [](void* ptr) { delete static_cast<VectorOfRangesT*>(ptr); });

  vtkmdiy::ContiguousAssigner assigner(/*num ranks*/ comm.size(),
                                       /*global-num-blocks*/ comm.size());
  vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds> decomposer(
    /*dim*/ 1, vtkmdiy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1); // each rank will have exactly 1 block.
  *master.block<VectorOfRangesT>(0) = v_ranges;

  vtkmdiy::RegularAllReducePartners all_reduce_partners(decomposer, /*k*/ 2);

  auto callback = [](
    VectorOfRangesT* data, const vtkmdiy::ReduceProxy& srp, const vtkmdiy::RegularMergePartners&) {
    const auto selfid = srp.gid();
    // 1. dequeue.
    std::vector<int> incoming;
    srp.incoming(incoming);
    for (const int gid : incoming)
    {
      if (gid != selfid)
      {
        VectorOfRangesT message;
        srp.dequeue(gid, message);

        // if the number of components we've seen so far is less than those
        // in the received message, resize so we can accommodate all components
        // in the message. If the message has fewer components, it has no
        // effect.
        data->resize(std::max(data->size(), message.size()));

        std::transform(
          message.begin(), message.end(), data->begin(), data->begin(), std::plus<vtkm::Range>());
      }
    }
    // 2. enqueue
    for (int cc = 0; cc < srp.out_link().size(); ++cc)
    {
      auto target = srp.out_link().target(cc);
      if (target.gid != selfid)
      {
        srp.enqueue(target, *data);
      }
    }
  };

  vtkmdiy::reduce(master, assigner, all_reduce_partners, callback);
  assert(master.size() == 1); // each rank will have exactly 1 block.

  return vtkm::cont::make_ArrayHandle(*master.block<VectorOfRangesT>(0), vtkm::CopyFlag::On);
}
} // namespace detail
}
} // namespace vtkm::cont
