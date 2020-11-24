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

#include <vtkm/cont/ErrorFilterExecution.h>
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
using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

template <typename AlgorithmType>
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
template <typename ResultType>
class VTKM_ALWAYS_EXPORT AdvectorBaseAlgorithm
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

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

  inline vtkm::cont::PartitionedDataSet GetOutput();

protected:
  virtual void ClearParticles()
  {
    this->Active.clear();
    this->Inactive.clear();
    this->Terminated.clear();
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

  void UpdateResultParticle(vtkm::Particle& p,
                            std::vector<vtkm::Particle>& I,
                            std::vector<vtkm::Particle>& T,
                            std::vector<vtkm::Particle>& A,
                            std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapI,
                            std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapA) const
  {
    if (p.Status.CheckTerminate())
      T.push_back(p);
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
        T.push_back(p);
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
    }
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
    VTKM_ASSERT(particles.size() == idsMap.size());
    if (particles.empty())
      return;

    this->Active.insert(this->Active.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  virtual void UpdateInactive(const std::vector<vtkm::Particle>& particles,
                              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    VTKM_ASSERT(particles.size() == idsMap.size());
    if (particles.empty())
      return;

    this->Inactive.insert(this->Inactive.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  virtual void UpdateTerminated(const std::vector<vtkm::Particle>& particles, vtkm::Id blockId)
  {
    if (particles.empty())
      return;

    for (const auto& t : particles)
      this->ParticleBlockIDsMap.erase(t.ID);
    auto& it = this->Terminated[blockId];
    it.insert(it.end(), particles.begin(), particles.end());
  }

  vtkm::Id UpdateResult(const ResultType& res, vtkm::Id blockId)
  {
    vtkm::Id n = res.Particles.GetNumberOfValues();
    auto portal = res.Particles.ReadPortal();

    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> idsMapI, idsMapA;
    std::vector<vtkm::Particle> I, T, A;

    for (vtkm::Id i = 0; i < n; i++)
    {
      vtkm::Particle p = portal.Get(i);
      this->UpdateResultParticle(p, I, T, A, idsMapI, idsMapA);
    }

    vtkm::Id numTerm = static_cast<vtkm::Id>(T.size());
    this->UpdateActive(A, idsMapA);
    this->UpdateInactive(I, idsMapI);
    this->UpdateTerminated(T, blockId);

    this->StoreResult(res, blockId);
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

  inline void StoreResult(const ResultType& res, vtkm::Id blockId);

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
  std::unordered_map<vtkm::Id, std::vector<vtkm::Particle>> Terminated;
};

}
}
} // namespace vtkm::filter::particleadvection


#ifndef vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_hxx
#include <vtkm/filter/particleadvection/AdvectorBaseAlgorithm.hxx>
#endif

#endif
