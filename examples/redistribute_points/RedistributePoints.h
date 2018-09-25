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
#include <vtkm/cont/Serialization.h>
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

  class ConcatenateFields
  {
  public:
    explicit ConcatenateFields(vtkm::Id totalSize) : TotalSize(totalSize), CurrentIdx(0) {}

    void Append(const vtkm::cont::Field& field)
    {
      VTKM_ASSERT(this->CurrentIdx + field.GetData().GetNumberOfValues() <= this->TotalSize);

      if (this->Field.GetData().GetNumberOfValues() == 0)
      {
        this->Field = field;
        field.GetData().CastAndCall(Allocator{}, this->Field, this->TotalSize);
      }
      else
      {
        VTKM_ASSERT(this->Field.GetName() == field.GetName() &&
                    this->Field.GetAssociation() == field.GetAssociation());
      }

      field.GetData().CastAndCall(Appender{}, this->Field, this->CurrentIdx);
      this->CurrentIdx += field.GetData().GetNumberOfValues();
    }

    const vtkm::cont::Field& GetResult() const
    {
      return this->Field;
    }

  private:
    struct Allocator
    {
      template <typename T, typename S>
      void operator()(const vtkm::cont::ArrayHandle<T, S>&,
                      vtkm::cont::Field& field,
                      vtkm::Id totalSize) const
      {
        vtkm::cont::ArrayHandle<T> init;
        init.Allocate(totalSize);
        field.SetData(init);
      }
    };

    struct Appender
    {
      template <typename T, typename S>
      void operator()(const vtkm::cont::ArrayHandle<T, S>& data,
                      vtkm::cont::Field& field,
                      vtkm::Id currentIdx) const
      {
        vtkm::cont::ArrayHandle<T> farray =
          field.GetData().template Cast<vtkm::cont::ArrayHandle<T>>();
        vtkm::cont::Algorithm::CopySubRange(data, 0, data.GetNumberOfValues(), farray, currentIdx);
      }
    };

    vtkm::Id TotalSize;
    vtkm::Id CurrentIdx;
    vtkm::cont::Field Field;
  };

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
          rp.enqueue(target, vtkm::filter::MakeSerializableDataSet(extractedDS, DerivedPolicy{}));
        }
        // clear our dataset.
        *block = vtkm::cont::DataSet();
      }
    }
    else
    {
      vtkm::Id numValues = 0;
      std::vector<vtkm::cont::DataSet> receives;
      for (int cc = 0; cc < rp.in_link().size(); ++cc)
      {
        auto target = rp.in_link().target(cc);
        if (rp.incoming(target.gid).size() > 0)
        {
          auto sds = vtkm::filter::MakeSerializableDataSet(DerivedPolicy{});
          rp.dequeue(target.gid, sds);
          receives.push_back(sds.DataSet);
          numValues += receives.back().GetCoordinateSystem(0).GetData().GetNumberOfValues();
        }
      }

      *block = vtkm::cont::DataSet();
      if (receives.size() == 1)
      {
        *block = receives[0];
      }
      else if (receives.size() > 1)
      {
        ConcatenateFields concatCoords(numValues);
        for (const auto& ds: receives)
        {
          concatCoords.Append(ds.GetCoordinateSystem(0));
        }
        block->AddCoordinateSystem(vtkm::cont::CoordinateSystem(
          concatCoords.GetResult().GetName(), concatCoords.GetResult().GetData()));

        for (vtkm::IdComponent i = 0; i < receives[0].GetNumberOfFields(); ++i)
        {
          ConcatenateFields concatField(numValues);
          for (const auto& ds: receives)
          {
            concatField.Append(ds.GetField(i));
          }
          block->AddField(concatField.GetResult());
        }
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
