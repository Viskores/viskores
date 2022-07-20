//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DataSetIntegrator_h
#define vtk_m_filter_particleadvection_DataSetIntegrator_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/EulerIntegrator.h>
#include <vtkm/worklet/particleadvection/IntegratorStatus.h>
#include <vtkm/worklet/particleadvection/RK4Integrator.h>
#include <vtkm/worklet/particleadvection/Stepper.h>

#include <vtkm/cont/internal/Variant.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(const std::vector<ParticleType>& v,
                const vtkm::filter::particleadvection::BoundsMap& boundsMap,
                const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , V(v)
  {
  }

  const vtkm::filter::particleadvection::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;

  std::vector<ParticleType> A, I, V;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> IdMapA, IdMapI;
  std::vector<vtkm::Id> TermIdx, TermID;
};

using DSIHelperInfoType = vtkm::cont::internal::Variant<DSIHelperInfo<vtkm::Particle>,
                                                        DSIHelperInfo<vtkm::ChargedParticle>>;

class DataSetIntegrator
{
protected:
  using VelocityFieldNameType = std::string;
  using ElectroMagneticFieldNameType = std::pair<std::string, std::string>;
  using FieldNameType =
    vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType>;

  using RType =
    vtkm::cont::internal::Variant<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>,
                                  vtkm::worklet::ParticleAdvectionResult<vtkm::ChargedParticle>,
                                  vtkm::worklet::StreamlineResult<vtkm::Particle>,
                                  vtkm::worklet::StreamlineResult<vtkm::ChargedParticle>>;

public:
  DataSetIntegrator(vtkm::Id id,
                    const FieldNameType& fieldName,
                    vtkm::filter::particleadvection::IntegrationSolverType solverType,
                    vtkm::filter::particleadvection::VectorFieldType vecFieldType,
                    vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
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
  virtual void Advect(DSIHelperInfoType& b,
                      vtkm::FloatDefault stepSize, //move these to member data(?)
                      vtkm::Id maxSteps)
  {
    if (b.GetIndex() == b.GetIndexOf<DSIHelperInfo<vtkm::Particle>>())
    {
      auto& bb = b.Get<DSIHelperInfo<vtkm::Particle>>();
      this->DoAdvect(bb, stepSize, maxSteps);
    }
    else if (b.GetIndex() == b.GetIndexOf<DSIHelperInfo<vtkm::ChargedParticle>>())
    {
      auto& bb = b.Get<DSIHelperInfo<vtkm::ChargedParticle>>();
      this->DoAdvect(bb, stepSize, maxSteps);
    }

    //b.CastAndCall([&] (auto& concrete) { this->DoAdvect(concrete, stepSize, maxSteps); });
  }

  template <typename ParticleType>
  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& ds) const;


protected:
  template <typename ParticleType, template <typename> class ResultType>
  VTKM_CONT void UpdateResult(const ResultType<ParticleType>& result,
                              DSIHelperInfo<ParticleType>& dsiInfo);

  VTKM_CONT bool IsParticleAdvectionResult() const
  {
    return this->AdvectionResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE;
  }

  VTKM_CONT bool IsStreamlineResult() const
  {
    return this->AdvectionResType == ParticleAdvectionResultType::STREAMLINE_TYPE;
  }

  VTKM_CONT virtual void DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) = 0;

  VTKM_CONT virtual void DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) = 0;

  template <typename ParticleType>
  VTKM_CONT void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                   DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType> FieldName;

  vtkm::Id Id;
  vtkm::filter::particleadvection::IntegrationSolverType SolverType;
  vtkm::filter::particleadvection::VectorFieldType VecFieldType;
  vtkm::filter::particleadvection::ParticleAdvectionResultType AdvectionResType =
    vtkm::filter::particleadvection::ParticleAdvectionResultType::UNKNOWN_TYPE;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;
  bool CopySeedArray = false;
  std::vector<RType> Results;
};

template <typename ParticleType>
VTKM_CONT void DataSetIntegrator::ClassifyParticles(
  const vtkm::cont::ArrayHandle<ParticleType>& particles,
  DSIHelperInfo<ParticleType>& dsiInfo) const
{
  dsiInfo.A.clear();
  dsiInfo.I.clear();
  dsiInfo.TermID.clear();
  dsiInfo.TermIdx.clear();
  dsiInfo.IdMapI.clear();
  dsiInfo.IdMapA.clear();

  auto portal = particles.WritePortal();
  vtkm::Id n = portal.GetNumberOfValues();

  for (vtkm::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);

    if (p.Status.CheckTerminate())
    {
      dsiInfo.TermIdx.push_back(i);
      dsiInfo.TermID.push_back(p.ID);
    }
    else
    {
      const auto& it = dsiInfo.ParticleBlockIDsMap.find(p.ID);
      VTKM_ASSERT(it != dsiInfo.ParticleBlockIDsMap.end());
      auto currBIDs = it->second;
      VTKM_ASSERT(!currBIDs.empty());

      std::vector<vtkm::Id> newIDs;
      if (p.Status.CheckSpatialBounds() && !p.Status.CheckTookAnySteps())
        newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
      else
        newIDs = dsiInfo.BoundsMap.FindBlocks(p.Pos, currBIDs);

      //reset the particle status.
      p.Status = vtkm::ParticleStatus();

      if (newIDs.empty()) //No blocks, we're done.
      {
        p.Status.SetTerminate();
        dsiInfo.TermIdx.push_back(i);
        dsiInfo.TermID.push_back(p.ID);
      }
      else
      {
        //If we have more than blockId, we want to minimize communication
        //and put any blocks owned by this rank first.
        if (newIDs.size() > 1)
        {
          for (auto idit = newIDs.begin(); idit != newIDs.end(); idit++)
          {
            vtkm::Id bid = *idit;
            if (dsiInfo.BoundsMap.FindRank(bid) == this->Rank)
            {
              newIDs.erase(idit);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        int dstRank = dsiInfo.BoundsMap.FindRank(newIDs[0]);
        if (dstRank == this->Rank)
        {
          dsiInfo.A.push_back(p);
          dsiInfo.IdMapA[p.ID] = newIDs;
        }
        else
        {
          dsiInfo.I.push_back(p);
          dsiInfo.IdMapI[p.ID] = newIDs;
        }
      }
      portal.Set(i, p);
    }
  }

  //Make sure we didn't miss anything. Every particle goes into a single bucket.
  VTKM_ASSERT(static_cast<std::size_t>(n) ==
              (dsiInfo.A.size() + dsiInfo.I.size() + dsiInfo.TermIdx.size()));
  VTKM_ASSERT(dsiInfo.TermIdx.size() == dsiInfo.TermID.size());
}

template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT void DataSetIntegrator::UpdateResult(const ResultType<ParticleType>& result,
                                               DSIHelperInfo<ParticleType>& dsiInfo)
{
  this->ClassifyParticles(result.Particles, dsiInfo);

  if (this->IsParticleAdvectionResult())
  {
    if (dsiInfo.TermIdx.empty())
      return;

    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;
    auto indicesAH = vtkm::cont::make_ArrayHandle(dsiInfo.TermIdx, vtkm::CopyFlag::Off);
    auto termPerm = vtkm::cont::make_ArrayHandlePermutation(indicesAH, result.Particles);

    vtkm::cont::ArrayHandle<ParticleType> termParticles;
    vtkm::cont::Algorithm::Copy(termPerm, termParticles);

    ResType termRes(termParticles);
    this->Results.push_back(termRes);
  }
  else if (this->IsStreamlineResult())
    this->Results.push_back(result);
}

template <typename ParticleType>
VTKM_CONT bool DataSetIntegrator::GetOutput(vtkm::cont::DataSet& ds) const
{
  std::size_t nResults = this->Results.size();
  if (nResults == 0)
    return false;

  if (this->IsParticleAdvectionResult())
  {
    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;

    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);
    for (const auto& vres : this->Results)
      allParticles.push_back(vres.Get<ResType>().Particles);

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
    using ResType = vtkm::worklet::StreamlineResult<ParticleType>;

    //Easy case with one result.
    if (nResults == 1)
    {
      const auto& res = this->Results[0].Get<ResType>();
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Positions));
      ds.SetCellSet(res.PolyLines);
    }
    else
    {
      std::vector<vtkm::Id> posOffsets(nResults, 0);
      vtkm::Id totalNumCells = 0, totalNumPts = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
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
        const auto& res = this->Results[i].Get<ResType>();
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
        const auto& res = this->Results[i].Get<ResType>();
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

#endif //vtk_m_filter_particleadvection_DataSetIntegrator_h
