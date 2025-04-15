//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "RedistributePoints.h"

#include <viskores/ImplicitFunction.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/AssignerPartitionedDataSet.h>
#include <viskores/cont/BoundsGlobalCompute.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Serialization.h>
#include <viskores/filter/entity_extraction/ExtractPoints.h>

#include <viskores/thirdparty/diy/diy.h>

namespace example
{

namespace internal
{

static viskoresdiy::ContinuousBounds convert(const viskores::Bounds& bds)
{
  viskoresdiy::ContinuousBounds result(3);
  result.min[0] = static_cast<float>(bds.X.Min);
  result.min[1] = static_cast<float>(bds.Y.Min);
  result.min[2] = static_cast<float>(bds.Z.Min);
  result.max[0] = static_cast<float>(bds.X.Max);
  result.max[1] = static_cast<float>(bds.Y.Max);
  result.max[2] = static_cast<float>(bds.Z.Max);
  return result;
}


template <typename FilterType>
class Redistributor
{
  const viskoresdiy::RegularDecomposer<viskoresdiy::ContinuousBounds>& Decomposer;
  const FilterType& Filter;

  viskores::cont::DataSet Extract(const viskores::cont::DataSet& input,
                                  const viskoresdiy::ContinuousBounds& bds) const
  {
    // extract points
    viskores::Box box(bds.min[0], bds.max[0], bds.min[1], bds.max[1], bds.min[2], bds.max[2]);

    viskores::filter::entity_extraction::ExtractPoints extractor;
    extractor.SetCompactPoints(true);
    extractor.SetImplicitFunction(box);
    return extractor.Execute(input);
  }

  class ConcatenateFields
  {
  public:
    explicit ConcatenateFields(viskores::Id totalSize)
      : TotalSize(totalSize)
      , CurrentIdx(0)
    {
    }

    void Append(const viskores::cont::Field& field)
    {
      VISKORES_ASSERT(this->CurrentIdx + field.GetNumberOfValues() <= this->TotalSize);

      if (this->Field.GetNumberOfValues() == 0)
      {
        // Copy metadata
        this->Field = field;
        // Reset array
        this->Field.SetData(field.GetData().NewInstanceBasic());
        // Preallocate array
        this->Field.GetData().Allocate(this->TotalSize);
      }
      else
      {
        VISKORES_ASSERT(this->Field.GetName() == field.GetName() &&
                        this->Field.GetAssociation() == field.GetAssociation());
      }

      field.GetData()
        .CastAndCallForTypes<VISKORES_DEFAULT_TYPE_LIST, VISKORES_DEFAULT_STORAGE_LIST>(
          Appender{}, this->Field, this->CurrentIdx);
      this->CurrentIdx += field.GetNumberOfValues();
    }

    const viskores::cont::Field& GetResult() const { return this->Field; }

  private:
    struct Appender
    {
      template <typename T, typename S>
      void operator()(const viskores::cont::ArrayHandle<T, S>& data,
                      viskores::cont::Field& field,
                      viskores::Id currentIdx) const
      {
        viskores::cont::ArrayHandle<T> farray =
          field.GetData().template AsArrayHandle<viskores::cont::ArrayHandle<T>>();
        viskores::cont::Algorithm::CopySubRange(
          data, 0, data.GetNumberOfValues(), farray, currentIdx);
      }
    };

    viskores::Id TotalSize;
    viskores::Id CurrentIdx;
    viskores::cont::Field Field;
  };

public:
  Redistributor(const viskoresdiy::RegularDecomposer<viskoresdiy::ContinuousBounds>& decomposer,
                const FilterType& filter)
    : Decomposer(decomposer)
    , Filter(filter)
  {
  }

  void operator()(viskores::cont::DataSet* block, const viskoresdiy::ReduceProxy& rp) const
  {
    if (rp.in_link().size() == 0)
    {
      if (block->GetNumberOfCoordinateSystems() > 0)
      {
        for (int cc = 0; cc < rp.out_link().size(); ++cc)
        {
          auto target = rp.out_link().target(cc);
          // let's get the bounding box for the target block.
          viskoresdiy::ContinuousBounds bds(3);
          this->Decomposer.fill_bounds(bds, target.gid);

          auto extractedDS = this->Extract(*block, bds);
          rp.enqueue(target, extractedDS);
        }
        // clear our dataset.
        *block = viskores::cont::DataSet();
      }
    }
    else
    {
      viskores::Id numValues = 0;
      std::vector<viskores::cont::DataSet> receives;
      for (int cc = 0; cc < rp.in_link().size(); ++cc)
      {
        auto target = rp.in_link().target(cc);
        if (rp.incoming(target.gid).size() > 0)
        {
          viskores::cont::DataSet incomingDS;
          rp.dequeue(target.gid, incomingDS);
          receives.push_back(incomingDS);
          numValues += receives.back().GetCoordinateSystem(0).GetNumberOfPoints();
        }
      }

      *block = viskores::cont::DataSet();
      if (receives.size() == 1)
      {
        *block = receives[0];
      }
      else if (receives.size() > 1)
      {
        ConcatenateFields concatCoords(numValues);
        for (const auto& ds : receives)
        {
          concatCoords.Append(ds.GetCoordinateSystem(0));
        }
        block->AddCoordinateSystem(viskores::cont::CoordinateSystem(
          concatCoords.GetResult().GetName(), concatCoords.GetResult().GetData()));

        for (viskores::IdComponent i = 0; i < receives[0].GetNumberOfFields(); ++i)
        {
          ConcatenateFields concatField(numValues);
          for (const auto& ds : receives)
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


VISKORES_CONT viskores::cont::PartitionedDataSet RedistributePoints::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();

  // let's first get the global bounds of the domain
  viskores::Bounds gbounds = viskores::cont::BoundsGlobalCompute(input);

  viskores::cont::AssignerPartitionedDataSet assigner(input.GetNumberOfPartitions());
  viskoresdiy::RegularDecomposer<viskoresdiy::ContinuousBounds> decomposer(
    /*dim*/ 3, internal::convert(gbounds), assigner.nblocks());

  viskoresdiy::Master master(
    comm,
    /*threads*/ 1,
    /*limit*/ -1,
    []() -> void* { return new viskores::cont::DataSet(); },
    [](void* ptr) { delete static_cast<viskores::cont::DataSet*>(ptr); });
  decomposer.decompose(comm.rank(), assigner, master);

  assert(static_cast<viskores::Id>(master.size()) == input.GetNumberOfPartitions());
  // let's populate local blocks
  master.foreach (
    [&input](viskores::cont::DataSet* ds, const viskoresdiy::Master::ProxyWithLink& proxy)
    {
      auto lid = proxy.master()->lid(proxy.gid());
      *ds = input.GetPartition(lid);
    });

  internal::Redistributor<RedistributePoints> redistributor(decomposer, *this);
  viskoresdiy::all_to_all(master, assigner, redistributor, /*k=*/2);

  viskores::cont::PartitionedDataSet result;
  master.foreach ([&result](viskores::cont::DataSet* ds, const viskoresdiy::Master::ProxyWithLink&)
                  { result.AppendPartition(*ds); });

  return result;
}

viskores::cont::DataSet RedistributePoints::DoExecute(const viskores::cont::DataSet&)
{
  throw viskores::cont::ErrorBadType("RedistributePoints requires PartitionedDataSet.");
}

} // namespace example
