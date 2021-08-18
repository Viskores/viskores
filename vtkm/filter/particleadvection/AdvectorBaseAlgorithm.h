//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_h
#define vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleMessenger.h>

#include <map>
#include <vector>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

namespace internal
{

//Helper class to store the different result types.
template <typename ResultType>
class ResultHelper;

//Specialization for ParticleAdvectionResult
using PAType = vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>;

template <>
class ResultHelper<PAType>
{
public:
  static void Store(std::map<vtkm::Id, std::vector<PAType>>& results,
                    const PAType& res,
                    vtkm::Id blockId,
                    const std::vector<vtkm::Id>& indices)
  {
    if (indices.empty())
      return;

    //Selected out the terminated particles and store them.
    vtkm::cont::ArrayHandle<vtkm::Particle> termParticles;
    auto indicesAH = vtkm::cont::make_ArrayHandle(indices, vtkm::CopyFlag::Off);
    auto perm = vtkm::cont::make_ArrayHandlePermutation(indicesAH, res.Particles);

    vtkm::cont::Algorithm::Copy(perm, termParticles);
    PAType termRes(termParticles);
    results[blockId].push_back(termRes);
  }

  static vtkm::cont::PartitionedDataSet GetOutput(
    const std::map<vtkm::Id, std::vector<PAType>>& results)
  {
    vtkm::cont::PartitionedDataSet output;
    for (const auto& it : results)
    {
      std::size_t nResults = it.second.size();
      if (nResults == 0)
        continue;

      std::vector<vtkm::cont::ArrayHandle<vtkm::Particle>> allParticles;
      allParticles.reserve(static_cast<std::size_t>(nResults));

      for (const auto& res : it.second)
        allParticles.push_back(res.Particles);

      vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
      vtkm::cont::ParticleArrayCopy(allParticles, pts);

      vtkm::cont::DataSet ds;
      vtkm::cont::CoordinateSystem outCoords("coordinates", pts);
      ds.AddCoordinateSystem(outCoords);

      //Create vertex cell set
      vtkm::Id numPoints = pts.GetNumberOfValues();
      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      ds.SetCellSet(cells);

      output.AppendPartition(ds);
    }

    return output;
  }
};

//Specialization for StreamlineResult
using SLType = vtkm::worklet::StreamlineResult<vtkm::Particle>;

template <>
class ResultHelper<SLType>
{
public:
  static void Store(std::map<vtkm::Id, std::vector<SLType>>& results,
                    const SLType& res,
                    vtkm::Id blockId,
                    const std::vector<vtkm::Id>& vtkmNotUsed(indices))
  {
    results[blockId].push_back(res);
  }

  static vtkm::cont::PartitionedDataSet GetOutput(
    const std::map<vtkm::Id, std::vector<SLType>>& results)
  {
    vtkm::cont::PartitionedDataSet output;
    for (const auto& it : results)
    {
      std::size_t nResults = it.second.size();
      if (nResults == 0)
        continue;

      vtkm::cont::DataSet ds;
      //Easy case with one result.
      if (nResults == 1)
      {
        ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", it.second[0].Positions));
        ds.SetCellSet(it.second[0].PolyLines);
      }
      else
      {
        //Append all the results into one data set.
        vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
        std::vector<vtkm::Id> posOffsets(nResults);

        const auto& res0 = it.second[0];
        vtkm::Id totalNumCells = res0.PolyLines.GetNumberOfCells();
        vtkm::Id totalNumPts = res0.Positions.GetNumberOfValues();

        posOffsets[0] = 0;
        for (std::size_t i = 1; i < nResults; i++)
        {
          const auto& res = it.second[i];
          posOffsets[i] = totalNumPts;
          totalNumPts += res.Positions.GetNumberOfValues();
          totalNumCells += res.PolyLines.GetNumberOfCells();
        }

        //Append all the points together.
        appendPts.Allocate(totalNumPts);
        for (std::size_t i = 0; i < nResults; i++)
        {
          const auto& res = it.second[i];
          // copy all values into appendPts starting at offset.
          vtkm::cont::Algorithm::CopySubRange(
            res.Positions, 0, res.Positions.GetNumberOfValues(), appendPts, posOffsets[i]);
        }
        vtkm::cont::CoordinateSystem outputCoords =
          vtkm::cont::CoordinateSystem("coordinates", appendPts);
        ds.AddCoordinateSystem(outputCoords);

        //Create polylines.
        std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
        std::size_t off = 0;
        for (std::size_t i = 0; i < nResults; i++)
        {
          const auto& res = it.second[i];
          vtkm::Id nCells = res.PolyLines.GetNumberOfCells();
          for (vtkm::Id j = 0; j < nCells; j++)
            numPtsPerCell[off++] = static_cast<vtkm::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
        }

        auto numPointsPerCellArray =
          vtkm::cont::make_ArrayHandle(numPtsPerCell, vtkm::CopyFlag::Off);

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
      output.AppendPartition(ds);
    }

    return output;
  }
};
};

template <typename DataSetIntegratorType, typename AlgorithmType>
vtkm::cont::PartitionedDataSet RunAlgo(const vtkm::filter::particleadvection::BoundsMap& boundsMap,
                                       const std::vector<DataSetIntegratorType>& dsi,
                                       vtkm::Id numSteps,
                                       vtkm::FloatDefault stepSize,
                                       const vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  AlgorithmType algo(boundsMap, dsi);

  algo.SetNumberOfSteps(numSteps);
  algo.SetStepSize(stepSize);
  algo.SetSeeds(seeds);
  algo.Go();
  return algo.GetOutput();
}

//
// Base class for particle advector
//
template <typename DataSetIntegratorType, typename ResultType>
class VTKM_ALWAYS_EXPORT AdvectorBaseAlgorithm
{
public:
  AdvectorBaseAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                        const std::vector<DataSetIntegratorType>& blocks)
    : Blocks(blocks)
    , BoundsMap(bm)
    , NumberOfSteps(0)
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
    , StepSize(0)
    , TotalNumParticles(0)
    , TotalNumTerminatedParticles(0)
  {
  }

  //Initialize ParticleAdvectorBase
  void SetStepSize(vtkm::FloatDefault stepSize) { this->StepSize = stepSize; }
  void SetNumberOfSteps(vtkm::Id numSteps) { this->NumberOfSteps = numSteps; }
  void SetSeeds(const vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
  {
    this->ClearParticles();

    vtkm::Id n = seeds.GetNumberOfValues();
    auto portal = seeds.ReadPortal();

    std::vector<std::vector<vtkm::Id>> blockIDs;
    std::vector<vtkm::Particle> particles;
    for (vtkm::Id i = 0; i < n; i++)
    {
      const vtkm::Particle p = portal.Get(i);
      std::vector<vtkm::Id> ids = this->BoundsMap.FindBlocks(p.Pos);
      if (!ids.empty() && this->BoundsMap.FindRank(ids[0]) == this->Rank)
      {
        particles.push_back(p);
        blockIDs.push_back(ids);
      }
    }

    this->SetSeedArray(particles, blockIDs);
  }

  //Advect all the particles.
  virtual void Go()
  {
    vtkm::filter::particleadvection::ParticleMessenger messenger(
      this->Comm, this->BoundsMap, 1, 128);

    vtkm::Id nLocal = static_cast<vtkm::Id>(this->Active.size() + this->Inactive.size());
    this->ComputeTotalNumParticles(nLocal);
    this->TotalNumTerminatedParticles = 0;

    while (this->TotalNumTerminatedParticles < this->TotalNumParticles)
    {
      std::vector<vtkm::Particle> v;
      vtkm::Id numTerm = 0, blockId = -1;
      if (GetActiveParticles(v, blockId))
      {
        const auto& block = this->GetDataSet(blockId);
        ResultType r;
        block.Advect(v, this->StepSize, this->NumberOfSteps, r);
        numTerm = this->UpdateResult(r, blockId);
      }

      vtkm::Id numTermMessages = 0;
      this->Communicate(messenger, numTerm, numTermMessages);

      this->TotalNumTerminatedParticles += (numTerm + numTermMessages);
      if (this->TotalNumTerminatedParticles > this->TotalNumParticles)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");
    }
  }

  vtkm::cont::PartitionedDataSet GetOutput() const
  {
    return internal::ResultHelper<ResultType>::GetOutput(this->Results);
  }

protected:
  virtual void ClearParticles()
  {
    this->Active.clear();
    this->Inactive.clear();
    this->ParticleBlockIDsMap.clear();
  }

  void ComputeTotalNumParticles(const vtkm::Id& numLocal)
  {
    long long total = static_cast<long long>(numLocal);
#ifdef VTKM_ENABLE_MPI
    MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(this->Comm.handle());
    MPI_Allreduce(MPI_IN_PLACE, &total, 1, MPI_LONG_LONG, MPI_SUM, mpiComm);
#endif
    this->TotalNumParticles = static_cast<vtkm::Id>(total);
  }

  const DataSetIntegratorType& GetDataSet(vtkm::Id id) const
  {
    for (const auto& it : this->Blocks)
      if (it.GetID() == id)
        return it;
    throw vtkm::cont::ErrorFilterExecution("Bad block");
  }

  virtual void SetSeedArray(const std::vector<vtkm::Particle>& particles,
                            const std::vector<std::vector<vtkm::Id>>& blockIds)
  {
    VTKM_ASSERT(particles.size() == blockIds.size());

    auto pit = particles.begin();
    auto bit = blockIds.begin();
    while (pit != particles.end() && bit != blockIds.end())
    {
      this->ParticleBlockIDsMap[pit->ID] = *bit;
      pit++;
      bit++;
    }

    this->Active.insert(this->Active.end(), particles.begin(), particles.end());
  }

  virtual bool GetActiveParticles(std::vector<vtkm::Particle>& particles, vtkm::Id& blockId)
  {
    particles.clear();
    blockId = -1;
    if (this->Active.empty())
      return false;

    blockId = this->ParticleBlockIDsMap[this->Active.front().ID][0];
    auto it = this->Active.begin();
    while (it != this->Active.end())
    {
      auto p = *it;
      if (blockId == this->ParticleBlockIDsMap[p.ID][0])
      {
        particles.push_back(p);
        it = this->Active.erase(it);
      }
      else
        it++;
    }

    return !particles.empty();
  }

  virtual void Communicate(vtkm::filter::particleadvection::ParticleMessenger& messenger,
                           vtkm::Id numLocalTerminations,
                           vtkm::Id& numTermMessages)
  {
    std::vector<vtkm::Particle> incoming;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> incomingIDs;
    numTermMessages = 0;
    messenger.Exchange(this->Inactive,
                       this->ParticleBlockIDsMap,
                       numLocalTerminations,
                       incoming,
                       incomingIDs,
                       numTermMessages,
                       this->GetBlockAndWait(numLocalTerminations));

    this->Inactive.clear();
    this->UpdateActive(incoming, incomingIDs);
  }

  virtual void UpdateActive(const std::vector<vtkm::Particle>& particles,
                            const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    this->Update(this->Active, particles, idsMap);
  }

  virtual void UpdateInactive(const std::vector<vtkm::Particle>& particles,
                              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    this->Update(this->Inactive, particles, idsMap);
  }

  void Update(std::vector<vtkm::Particle>& arr,
              const std::vector<vtkm::Particle>& particles,
              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    VTKM_ASSERT(particles.size() == idsMap.size());

    arr.insert(arr.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  void UpdateTerminated(const vtkm::cont::ArrayHandle<vtkm::Particle>& particles,
                        const std::vector<vtkm::Id>& idxTerm)
  {
    auto portal = particles.ReadPortal();
    for (const auto& idx : idxTerm)
      this->ParticleBlockIDsMap.erase(portal.Get(idx).ID);
  }

  void ClassifyParticles(const vtkm::cont::ArrayHandle<vtkm::Particle>& particles,
                         std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapA,
                         std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapI,
                         std::vector<vtkm::Particle>& A,
                         std::vector<vtkm::Particle>& I,
                         std::vector<vtkm::Id>& termIdx) const
  {
    A.clear();
    I.clear();
    termIdx.clear();
    idsMapI.clear();
    idsMapA.clear();

    auto portal = particles.WritePortal();
    vtkm::Id n = portal.GetNumberOfValues();

    for (vtkm::Id i = 0; i < n; i++)
    {
      auto p = portal.Get(i);

      if (p.Status.CheckTerminate())
        termIdx.push_back(i);
      else
      {
        const auto& it = this->ParticleBlockIDsMap.find(p.ID);
        VTKM_ASSERT(it != this->ParticleBlockIDsMap.end());
        auto currBIDs = it->second;
        VTKM_ASSERT(!currBIDs.empty());

        std::vector<vtkm::Id> newIDs;
        if (p.Status.CheckSpatialBounds() && !p.Status.CheckTookAnySteps())
          newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
        else
          newIDs = this->BoundsMap.FindBlocks(p.Pos, currBIDs);

        //reset the particle status.
        p.Status = vtkm::ParticleStatus();

        if (newIDs.empty()) //No blocks, we're done.
        {
          p.Status.SetTerminate();
          termIdx.push_back(i);
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
              if (this->BoundsMap.FindRank(bid) == this->Rank)
              {
                newIDs.erase(idit);
                newIDs.insert(newIDs.begin(), bid);
                break;
              }
            }
          }

          int dstRank = this->BoundsMap.FindRank(newIDs[0]);
          if (dstRank == this->Rank)
          {
            A.push_back(p);
            idsMapA[p.ID] = newIDs;
          }
          else
          {
            I.push_back(p);
            idsMapI[p.ID] = newIDs;
          }
        }
        portal.Set(i, p);
      }
    }

    //Make sure we didn't miss anything. Every particle goes into a single bucket.
    VTKM_ASSERT(static_cast<std::size_t>(n) == (A.size() + I.size() + termIdx.size()));
  }


  vtkm::Id UpdateResult(ResultType& res, vtkm::Id blockId)
  {
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> idsMapI, idsMapA;
    std::vector<vtkm::Particle> A, I;
    std::vector<vtkm::Id> termIdx;
    this->ClassifyParticles(res.Particles, idsMapA, idsMapI, A, I, termIdx);

    //Update active, inactive and terminated
    this->UpdateActive(A, idsMapA);
    this->UpdateInactive(I, idsMapI);

    vtkm::Id numTerm = static_cast<vtkm::Id>(termIdx.size());
    if (numTerm > 0)
      this->UpdateTerminated(res.Particles, termIdx);

    internal::ResultHelper<ResultType>::Store(this->Results, res, blockId, termIdx);

    return numTerm;
  }

  virtual bool GetBlockAndWait(const vtkm::Id& numLocalTerm)
  {
    //There are only two cases where blocking would deadlock.
    //1. There are active particles.
    //2. numLocalTerm + this->TotalNumberOfTerminatedParticles == this->TotalNumberOfParticles
    //So, if neither are true, we can safely block and wait for communication to come in.

    if (this->Active.empty() && this->Inactive.empty() &&
        (numLocalTerm + this->TotalNumTerminatedParticles < this->TotalNumParticles))
    {
      return true;
    }

    return false;
  }

  //Member data
  std::vector<vtkm::Particle> Active;
  std::vector<DataSetIntegratorType> Blocks;
  vtkm::filter::particleadvection::BoundsMap BoundsMap;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  std::vector<vtkm::Particle> Inactive;
  vtkm::Id NumberOfSteps;
  vtkm::Id NumRanks;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
  vtkm::Id Rank;
  std::map<vtkm::Id, std::vector<ResultType>> Results;
  vtkm::FloatDefault StepSize;
  vtkm::Id TotalNumParticles;
  vtkm::Id TotalNumTerminatedParticles;
};

}
}
} // namespace vtkm::filter::particleadvection

#endif
