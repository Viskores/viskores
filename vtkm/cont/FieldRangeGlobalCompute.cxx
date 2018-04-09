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
#include <vtkm/cont/FieldRangeGlobalCompute.h>

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/diy/Serialization.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include VTKM_DIY(diy/decomposition.hpp)
#include VTKM_DIY(diy/master.hpp)
#include VTKM_DIY(diy/partners/all-reduce.hpp)
#include VTKM_DIY(diy/reduce.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace vtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::AssociationEnum assoc)
{
  return detail::FieldRangeGlobalComputeImpl(
    dataset, name, assoc, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::MultiBlock& multiblock,
  const std::string& name,
  vtkm::cont::Field::AssociationEnum assoc)
{
  return detail::FieldRangeGlobalComputeImpl(
    multiblock, name, assoc, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
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

  using ArrayHandleT = vtkm::cont::ArrayHandle<vtkm::Range>;

  diy::Master master(comm,
                     1,
                     -1,
                     []() -> void* { return new ArrayHandleT(); },
                     [](void* ptr) { delete static_cast<ArrayHandleT*>(ptr); });

  diy::ContiguousAssigner assigner(/*num ranks*/ comm.size(), /*global-num-blocks*/ comm.size());
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1); // each rank will have exactly 1 block.
  *master.block<ArrayHandleT>(0) = ranges;

  diy::RegularAllReducePartners all_reduce_partners(decomposer, /*k*/ 2);

  auto callback =
    [](ArrayHandleT* data, const diy::ReduceProxy& srp, const diy::RegularMergePartners&) {
      const auto selfid = srp.gid();
      // 1. dequeue.
      std::vector<int> incoming;
      srp.incoming(incoming);
      for (const int gid : incoming)
      {
        if (gid != selfid)
        {
          ArrayHandleT message;
          srp.dequeue(gid, message);
          *data = vtkm::cont::detail::MergeRanges(*data, message);
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

  diy::reduce(master, assigner, all_reduce_partners, callback);
  assert(master.size() == 1); // each rank will have exactly 1 block.
  return *master.block<ArrayHandleT>(0);
}
} // namespace detail
}
} // namespace vtkm::cont
