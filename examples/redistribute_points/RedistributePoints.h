//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/ImplicitFunction.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/AssignerMultiBlock.h>
#include <vtkm/cont/BoundsGlobalCompute.h>
#include <vtkm/cont/diy/Serialization.h>
#include <vtkm/filter/ExtractPoints.h>
#include <vtkm/filter/Filter.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/decomposition.hpp)
#include VTKM_DIY(diy/link.hpp)
#include VTKM_DIY(diy/master.hpp)
#include VTKM_DIY(diy/proxy.hpp)
#include VTKM_DIY(diy/reduce.hpp)
#include VTKM_DIY(diy/reduce-operations.hpp)
#include VTKM_DIY(diy/types.hpp)
VTKM_THIRDPARTY_POST_INCLUDE

namespace example
{

namespace internal
{

static diy::ContinuousBounds convert(const vtkm::Bounds& bds)
{
  diy::ContinuousBounds result;
  result.min[0] = static_cast<float>(bds.X.Min);
  result.min[1] = static_cast<float>(bds.Y.Min);
  result.min[2] = static_cast<float>(bds.Z.Min);
  result.max[0] = static_cast<float>(bds.X.Max);
  result.max[1] = static_cast<float>(bds.Y.Max);
  result.max[2] = static_cast<float>(bds.Z.Max);
  return result;
}


template <typename DerivedPolicy>
class Redistributor
{
  const diy::RegularDecomposer<diy::ContinuousBounds>& Decomposer;
  const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;

  vtkm::cont::DataSet Extract(const vtkm::cont::DataSet& input, const diy::ContinuousBounds& bds) const
  {
    // extract points
    vtkm::Box box(bds.min[0], bds.max[0], bds.min[1], bds.max[1], bds.min[2], bds.max[2]);

    vtkm::filter::ExtractPoints extractor;
    extractor.SetCompactPoints(true);
    extractor.SetImplicitFunction(vtkm::cont::make_ImplicitFunctionHandle(box));
    return extractor.Execute(input, this->Policy);
  }


public:
  Redistributor(const diy::RegularDecomposer<diy::ContinuousBounds>& decomposer,
      const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
    : Decomposer(decomposer), Policy(policy)
  {
  }

  void operator()(vtkm::cont::DataSet* block, const diy::ReduceProxy& rp) const
  {
    if (rp.in_link().size() == 0)
    {
      if (block->GetNumberOfCoordinateSystems() > 0)
      {
        for (int cc = 0; cc < rp.out_link().size(); ++cc)
        {
          auto target = rp.out_link().target(cc);
          // let's get the bounding box for the target block.
          diy::ContinuousBounds bds;
          this->Decomposer.fill_bounds(bds, target.gid);

          auto extractedDS = this->Extract(*block, bds);
          const auto inputAH = extractedDS.GetCoordinateSystem(0).GetData();
          vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> outputAH;
          outputAH.Allocate(inputAH.GetNumberOfValues());
          vtkm::cont::Algorithm::Copy(inputAH, outputAH);
          rp.enqueue(target, outputAH);
        }
        // clear our dataset.
        *block = vtkm::cont::DataSet();
      }
    }
    else
    {
      vtkm::Id numValues = 0;
      std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>> received_arrays;
      for (int cc = 0; cc < rp.in_link().size(); ++cc)
      {
        auto target = rp.in_link().target(cc);
        if (rp.incoming(target.gid).size() > 0)
        {
          vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> incomingAH;
          rp.dequeue(target.gid, incomingAH);
          received_arrays.push_back(incomingAH);
          numValues += incomingAH.GetNumberOfValues();
        }
      }

      *block = vtkm::cont::DataSet();
      if (received_arrays.size() == 1)
      {
        auto coords = vtkm::cont::CoordinateSystem("coords", received_arrays[0]);
        block->AddCoordinateSystem(coords);
      }
      else if (received_arrays.size() > 1)
      {
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> coordsAH;
        coordsAH.Allocate(numValues);
        vtkm::Id offset = 0;
        for (const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& receivedAH : received_arrays)
        {
          vtkm::cont::Algorithm::CopySubRange(receivedAH, 0, receivedAH.GetNumberOfValues(), coordsAH, offset);
          offset += receivedAH.GetNumberOfValues();
        }
        block->AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", coordsAH));
      }
    }
  }
};

} // namespace example::internal


class RedistributePoints : public vtkm::filter::Filter<RedistributePoints>
{
public:
  VTKM_CONT
  RedistributePoints() {}

  VTKM_CONT
  ~RedistributePoints() {}

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::MultiBlock PrepareForExecution(
        const vtkm::cont::MultiBlock& input,
        const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
};

template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::MultiBlock
RedistributePoints::PrepareForExecution(
  const vtkm::cont::MultiBlock& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  // let's first get the global bounds of the domain
  vtkm::Bounds gbounds = vtkm::cont::BoundsGlobalCompute(input);

  vtkm::cont::AssignerMultiBlock assigner(input.GetNumberOfBlocks());
  diy::RegularDecomposer<diy::ContinuousBounds> decomposer(/*dim*/3, internal::convert(gbounds), assigner.nblocks());

  diy::Master master(comm,
      /*threads*/ 1,
      /*limit*/ -1,
      []() -> void* { return new vtkm::cont::DataSet(); },
      [](void*ptr) { delete static_cast<vtkm::cont::DataSet*>(ptr); });
  decomposer.decompose(comm.rank(), assigner, master);

  assert(static_cast<vtkm::Id>(master.size()) == input.GetNumberOfBlocks());
  // let's populate local blocks
  master.foreach(
      [&input](vtkm::cont::DataSet* ds, const diy::Master::ProxyWithLink& proxy) {
      auto lid = proxy.master()->lid(proxy.gid());
      *ds = input.GetBlock(lid);
      });

  internal::Redistributor<DerivedPolicy> redistributor(decomposer, policy);
  diy::all_to_all(master, assigner, redistributor, /*k=*/2);

  vtkm::cont::MultiBlock result;
  master.foreach(
      [&result](vtkm::cont::DataSet* ds, const diy::Master::ProxyWithLink&) {
      result.AddBlock(*ds);
      });

  return result;
}

} // namespace example
