//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/ImplicitFunction.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/BoundsGlobalCompute.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/filter/ExtractPoints.h>
#include <vtkm/filter/Filter.h>

#include <vtkm/thirdparty/diy/diy.h>

namespace example
{

namespace internal
{

static vtkmdiy::ContinuousBounds convert(const vtkm::Bounds& bds)
{
  vtkmdiy::ContinuousBounds result(3);
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
  const vtkmdiy::RegularDecomposer<vtkmdiy::ContinuousBounds>& Decomposer;
  const FilterType& Filter;

  vtkm::cont::DataSet Extract(const vtkm::cont::DataSet& input,
                              const vtkmdiy::ContinuousBounds& bds) const
  {
    // extract points
    vtkm::Box box(bds.min[0], bds.max[0], bds.min[1], bds.max[1], bds.min[2], bds.max[2]);

    vtkm::filter::ExtractPoints extractor;
    extractor.SetCompactPoints(true);
    extractor.SetImplicitFunction(box);
    return extractor.Execute(input);
  }

  class ConcatenateFields
  {
  public:
    explicit ConcatenateFields(vtkm::Id totalSize)
      : TotalSize(totalSize)
      , CurrentIdx(0)
    {
    }

    void Append(const vtkm::cont::Field& field)
    {
      VTKM_ASSERT(this->CurrentIdx + field.GetNumberOfValues() <= this->TotalSize);

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
        VTKM_ASSERT(this->Field.GetName() == field.GetName() &&
                    this->Field.GetAssociation() == field.GetAssociation());
      }

      field.GetData().CastAndCallForTypes<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(
        Appender{}, this->Field, this->CurrentIdx);
      this->CurrentIdx += field.GetNumberOfValues();
    }

    const vtkm::cont::Field& GetResult() const { return this->Field; }

  private:
    struct Appender
    {
      template <typename T, typename S>
      void operator()(const vtkm::cont::ArrayHandle<T, S>& data,
                      vtkm::cont::Field& field,
                      vtkm::Id currentIdx) const
      {
        vtkm::cont::ArrayHandle<T> farray =
          field.GetData().template AsArrayHandle<vtkm::cont::ArrayHandle<T>>();
        vtkm::cont::Algorithm::CopySubRange(data, 0, data.GetNumberOfValues(), farray, currentIdx);
      }
    };

    vtkm::Id TotalSize;
    vtkm::Id CurrentIdx;
    vtkm::cont::Field Field;
  };

public:
  Redistributor(const vtkmdiy::RegularDecomposer<vtkmdiy::ContinuousBounds>& decomposer,
                const FilterType& filter)
    : Decomposer(decomposer)
    , Filter(filter)
  {
  }

  void operator()(vtkm::cont::DataSet* block, const vtkmdiy::ReduceProxy& rp) const
  {
    if (rp.in_link().size() == 0)
    {
      if (block->GetNumberOfCoordinateSystems() > 0)
      {
        for (int cc = 0; cc < rp.out_link().size(); ++cc)
        {
          auto target = rp.out_link().target(cc);
          // let's get the bounding box for the target block.
          vtkmdiy::ContinuousBounds bds(3);
          this->Decomposer.fill_bounds(bds, target.gid);

          auto extractedDS = this->Extract(*block, bds);
          rp.enqueue(target, vtkm::filter::MakeSerializableDataSet(extractedDS, this->Filter));
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
          auto sds = vtkm::filter::MakeSerializableDataSet(this->Filter);
          rp.dequeue(target.gid, sds);
          receives.push_back(sds.DataSet);
          numValues += receives.back().GetCoordinateSystem(0).GetNumberOfPoints();
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
        for (const auto& ds : receives)
        {
          concatCoords.Append(ds.GetCoordinateSystem(0));
        }
        block->AddCoordinateSystem(vtkm::cont::CoordinateSystem(
          concatCoords.GetResult().GetName(), concatCoords.GetResult().GetData()));

        for (vtkm::IdComponent i = 0; i < receives[0].GetNumberOfFields(); ++i)
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


class RedistributePoints : public vtkm::filter::Filter<RedistributePoints>
{
public:
  VTKM_CONT
  RedistributePoints() {}

  VTKM_CONT
  ~RedistributePoints() {}

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
};

template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet RedistributePoints::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  // let's first get the global bounds of the domain
  vtkm::Bounds gbounds = vtkm::cont::BoundsGlobalCompute(input);

  vtkm::cont::AssignerPartitionedDataSet assigner(input.GetNumberOfPartitions());
  vtkmdiy::RegularDecomposer<vtkmdiy::ContinuousBounds> decomposer(
    /*dim*/ 3, internal::convert(gbounds), assigner.nblocks());

  vtkmdiy::Master master(
    comm,
    /*threads*/ 1,
    /*limit*/ -1,
    []() -> void* { return new vtkm::cont::DataSet(); },
    [](void* ptr) { delete static_cast<vtkm::cont::DataSet*>(ptr); });
  decomposer.decompose(comm.rank(), assigner, master);

  assert(static_cast<vtkm::Id>(master.size()) == input.GetNumberOfPartitions());
  // let's populate local blocks
  master.foreach ([&input](vtkm::cont::DataSet* ds, const vtkmdiy::Master::ProxyWithLink& proxy) {
    auto lid = proxy.master()->lid(proxy.gid());
    *ds = input.GetPartition(lid);
  });

  internal::Redistributor<RedistributePoints> redistributor(decomposer, *this);
  vtkmdiy::all_to_all(master, assigner, redistributor, /*k=*/2);

  vtkm::cont::PartitionedDataSet result;
  master.foreach ([&result](vtkm::cont::DataSet* ds, const vtkmdiy::Master::ProxyWithLink&) {
    result.AppendPartition(*ds);
  });

  return result;
}

} // namespace example
