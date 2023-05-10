//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_DataSetIntegrator_h
#define vtk_m_filter_flow_internal_DataSetIntegrator_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/worklet/EulerIntegrator.h>
#include <vtkm/filter/flow/worklet/IntegratorStatus.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/RK4Integrator.h>
#include <vtkm/filter/flow/worklet/Stepper.h>

#include <vtkm/cont/Variant.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(const std::vector<ParticleType>& v,
                const vtkm::filter::flow::internal::BoundsMap& boundsMap,
                const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , Particles(v)
  {
  }

  struct ParticleBlockIds
  {
    void Clear()
    {
      this->Particles.clear();
      this->BlockIDs.clear();
    }

    void Add(const ParticleType& p, const std::vector<vtkm::Id>& bids)
    {
      this->Particles.emplace_back(p);
      this->BlockIDs[p.GetID()] = std::move(bids);
    }

    std::vector<ParticleType> Particles;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> BlockIDs;
  };

  void Clear()
  {
    this->InBounds.Clear();
    this->OutOfBounds.Clear();
    this->TermIdx.clear();
    this->TermID.clear();
  }

  void Validate(vtkm::Id num)
  {
    //Make sure we didn't miss anything. Every particle goes into a single bucket.
    if ((static_cast<std::size_t>(num) !=
         (this->InBounds.Particles.size() + this->OutOfBounds.Particles.size() +
          this->TermIdx.size())) ||
        (this->InBounds.Particles.size() != this->InBounds.BlockIDs.size()) ||
        (this->OutOfBounds.Particles.size() != this->OutOfBounds.BlockIDs.size()) ||
        (this->TermIdx.size() != this->TermID.size()))
    {
      throw vtkm::cont::ErrorFilterExecution("Particle count mismatch after classification");
    }
  }

  void AddTerminated(vtkm::Id idx, vtkm::Id pID)
  {
    this->TermIdx.emplace_back(idx);
    this->TermID.emplace_back(pID);
  }

  const vtkm::filter::flow::internal::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;

  ParticleBlockIds InBounds;
  ParticleBlockIds OutOfBounds;
  std::vector<ParticleType> Particles;
  std::vector<vtkm::Id> TermID;
  std::vector<vtkm::Id> TermIdx;
};

using DSIHelperInfoType =
  vtkm::cont::Variant<DSIHelperInfo<vtkm::Particle>, DSIHelperInfo<vtkm::ChargedParticle>>;

template <typename Derived>
class DataSetIntegrator
{
public:
  using VelocityFieldNameType = std::string;
  using ElectroMagneticFieldNameType = std::pair<std::string, std::string>;

protected:
  using FieldNameType = vtkm::cont::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType>;

  using RType =
    vtkm::cont::Variant<vtkm::worklet::flow::ParticleAdvectionResult<vtkm::Particle>,
                        vtkm::worklet::flow::ParticleAdvectionResult<vtkm::ChargedParticle>,
                        vtkm::worklet::flow::StreamlineResult<vtkm::Particle>,
                        vtkm::worklet::flow::StreamlineResult<vtkm::ChargedParticle>>;

public:
  DataSetIntegrator(vtkm::Id id,
                    const FieldNameType& fieldName,
                    vtkm::filter::flow::IntegrationSolverType solverType,
                    vtkm::filter::flow::VectorFieldType vecFieldType,
                    vtkm::filter::flow::FlowResultType resultType)
    : FieldName(fieldName)
    , Id(id)
    , SolverType(solverType)
    , VecFieldType(vecFieldType)
    , AdvectionResType(resultType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  VTKM_CONT
  void Advect(DSIHelperInfoType& b,
              vtkm::FloatDefault stepSize, //move these to member data(?)
              vtkm::Id maxSteps)
  {
    Derived* inst = static_cast<Derived*>(this);

    //Cast the DSIHelperInfo<ParticleType> to the concrete type and call DoAdvect.
    b.CastAndCall([&](auto& concrete) { inst->DoAdvect(concrete, stepSize, maxSteps); });
  }

  template <typename ParticleType>
  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& ds) const;


protected:
  template <typename ParticleType, template <typename> class ResultType>
  VTKM_CONT void UpdateResult(const ResultType<ParticleType>& result,
                              DSIHelperInfo<ParticleType>& dsiInfo);

  VTKM_CONT bool IsParticleAdvectionResult() const
  {
    return this->AdvectionResType == FlowResultType::PARTICLE_ADVECT_TYPE;
  }

  VTKM_CONT bool IsStreamlineResult() const
  {
    return this->AdvectionResType == FlowResultType::STREAMLINE_TYPE;
  }

  template <typename ParticleType>
  VTKM_CONT inline void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                          DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  vtkm::cont::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType> FieldName;

  vtkm::Id Id;
  vtkm::filter::flow::IntegrationSolverType SolverType;
  vtkm::filter::flow::VectorFieldType VecFieldType;
  vtkm::filter::flow::FlowResultType AdvectionResType =
    vtkm::filter::flow::FlowResultType::UNKNOWN_TYPE;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;
  bool CopySeedArray = false;
  std::vector<RType> Results;
};

template <typename Derived>
template <typename ParticleType>
VTKM_CONT inline void DataSetIntegrator<Derived>::ClassifyParticles(
  const vtkm::cont::ArrayHandle<ParticleType>& particles,
  DSIHelperInfo<ParticleType>& dsiInfo) const
{
  /*
 each particle: --> T,I,A
 if T: update TermIdx, TermID
 if A: update IdMapA;
 if I: update IdMapI;
   */
  dsiInfo.Clear();

  auto portal = particles.WritePortal();
  vtkm::Id n = portal.GetNumberOfValues();

  for (vtkm::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);

    //Terminated.
    if (p.GetStatus().CheckTerminate())
    {
      dsiInfo.AddTerminated(i, p.GetID());
    }
    else
    {
      //Didn't terminate.
      //Get the blockIDs.
      const auto& it = dsiInfo.ParticleBlockIDsMap.find(p.GetID());
      VTKM_ASSERT(it != dsiInfo.ParticleBlockIDsMap.end());
      auto currBIDs = it->second;
      VTKM_ASSERT(!currBIDs.empty());

      std::vector<vtkm::Id> newIDs;
      if (p.GetStatus().CheckSpatialBounds() && !p.GetStatus().CheckTookAnySteps())
      {
        //particle is OUTSIDE but didn't take any steps.
        //this means that the particle wasn't in the block.
        //assign newIDs to currBIDs[1:]
        newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
      }
      else
      {
        //Otherwise, get new blocks from the current position.
        newIDs = dsiInfo.BoundsMap.FindBlocks(p.GetPosition(), currBIDs);
      }

      //reset the particle status.
      p.GetStatus() = vtkm::ParticleStatus();

      if (newIDs.empty()) //No blocks, we're done.
      {
        p.GetStatus().SetTerminate();
        dsiInfo.AddTerminated(i, p.GetID());
      }
      else
      {
        //If we have more than one blockId, we want to minimize communication
        //and put any blocks owned by this rank first.
        if (newIDs.size() > 1)
        {
          for (auto idit = newIDs.begin(); idit != newIDs.end(); idit++)
          {
            vtkm::Id bid = *idit;
            auto ranks = dsiInfo.BoundsMap.FindRank(bid);
            if (std::find(ranks.begin(), ranks.end(), this->Rank) != ranks.end())
            {
              newIDs.erase(idit);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        dsiInfo.OutOfBounds.Add(p, newIDs);
      }
    }
    portal.Set(i, p);
  }

  //Make sure everything is copacetic.
  dsiInfo.Validate(n);
}

template <typename Derived>
template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT inline void DataSetIntegrator<Derived>::UpdateResult(
  const ResultType<ParticleType>& result,
  DSIHelperInfo<ParticleType>& dsiInfo)
{
  this->ClassifyParticles(result.Particles, dsiInfo);

  if (this->IsParticleAdvectionResult())
  {
    if (dsiInfo.TermIdx.empty())
      return;

    using ResType = vtkm::worklet::flow::ParticleAdvectionResult<ParticleType>;
    auto indicesAH = vtkm::cont::make_ArrayHandle(dsiInfo.TermIdx, vtkm::CopyFlag::Off);
    auto termPerm = vtkm::cont::make_ArrayHandlePermutation(indicesAH, result.Particles);

    vtkm::cont::ArrayHandle<ParticleType> termParticles;
    vtkm::cont::Algorithm::Copy(termPerm, termParticles);

    ResType termRes(termParticles);
    this->Results.emplace_back(termRes);
  }
  else if (this->IsStreamlineResult())
    this->Results.emplace_back(result);
}

template <typename Derived>
template <typename ParticleType>
VTKM_CONT inline bool DataSetIntegrator<Derived>::GetOutput(vtkm::cont::DataSet& ds) const
{
  std::size_t nResults = this->Results.size();
  if (nResults == 0)
    return false;

  if (this->IsParticleAdvectionResult())
  {
    using ResType = vtkm::worklet::flow::ParticleAdvectionResult<ParticleType>;

    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);
    for (const auto& vres : this->Results)
      allParticles.emplace_back(vres.template Get<ResType>().Particles);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
    vtkm::cont::ParticleArrayCopy(allParticles, pts);

    vtkm::Id numPoints = pts.GetNumberOfValues();
    if (numPoints > 0)
    {
      //Create coordinate system and vertex cell set.
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", pts));

      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      ds.SetCellSet(cells);
    }
  }
  else if (this->IsStreamlineResult())
  {
    using ResType = vtkm::worklet::flow::StreamlineResult<ParticleType>;

    //Easy case with one result.
    if (nResults == 1)
    {
      const auto& res = this->Results[0].template Get<ResType>();
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Positions));
      ds.SetCellSet(res.PolyLines);
    }
    else
    {
      std::vector<vtkm::Id> posOffsets(nResults, 0);
      vtkm::Id totalNumCells = 0, totalNumPts = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].template Get<ResType>();
        if (i == 0)
          posOffsets[i] = 0;
        else
          posOffsets[i] = totalNumPts;

        totalNumPts += res.Positions.GetNumberOfValues();
        totalNumCells += res.PolyLines.GetNumberOfCells();
      }

      //Append all the points together.
      vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
      appendPts.Allocate(totalNumPts);
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].template Get<ResType>();
        // copy all values into appendPts starting at offset.
        vtkm::cont::Algorithm::CopySubRange(
          res.Positions, 0, res.Positions.GetNumberOfValues(), appendPts, posOffsets[i]);
      }
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", appendPts));

      //Create polylines.
      std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
      std::size_t off = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].template Get<ResType>();
        vtkm::Id nCells = res.PolyLines.GetNumberOfCells();
        for (vtkm::Id j = 0; j < nCells; j++)
          numPtsPerCell[off++] = static_cast<vtkm::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
      }

      auto numPointsPerCellArray = vtkm::cont::make_ArrayHandle(numPtsPerCell, vtkm::CopyFlag::Off);

      vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
      vtkm::Id connectivityLen =
        vtkm::cont::Algorithm::ScanExclusive(numPointsPerCellArray, cellIndex);
      vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
      vtkm::cont::ArrayCopy(connCount, connectivity);

      vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
      auto polyLineShape = vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(
        vtkm::CELL_SHAPE_POLY_LINE, totalNumCells);
      vtkm::cont::ArrayCopy(polyLineShape, cellTypes);
      auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numPointsPerCellArray);

      vtkm::cont::CellSetExplicit<> polyLines;
      polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
      ds.SetCellSet(polyLines);
    }
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution("Unsupported ParticleAdvectionResultType");
  }

  return true;
}

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegrator_h
