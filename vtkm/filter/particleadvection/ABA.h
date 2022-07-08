//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ABA_h
#define vtk_m_filter_ABA_h

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DSI.h>
#include <vtkm/filter/particleadvection/ParticleMessenger.h>


namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename DSIType, template <typename> class ResultType, typename ParticleType>
class ABA
{
public:
  ABA(const vtkm::filter::particleadvection::BoundsMap& bm, std::vector<DSIType*>& blocks)
    : Blocks(blocks)
    , BoundsMap(bm)
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
  {
  }

  void Execute(vtkm::Id numSteps,
               vtkm::FloatDefault stepSize,
               const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->SetNumberOfSteps(numSteps);
    this->SetStepSize(stepSize);
    this->SetSeeds(seeds);
    this->Go();
  }

  vtkm::cont::PartitionedDataSet GetOutput()
  {
    vtkm::cont::PartitionedDataSet output;

    for (const auto& b : this->Blocks)
    {
      vtkm::cont::DataSet ds;
      if (b->template GetOutput<ParticleType>(ds))
        output.AppendPartition(ds);
    }

    //std::cout<<"GetOutput: "<<__FILE__<<" "<<__LINE__<<std::endl;
    //output.PrintSummary(std::cout);

    return output;
  }

  void SetStepSize(vtkm::FloatDefault stepSize) { this->StepSize = stepSize; }
  void SetNumberOfSteps(vtkm::Id numSteps) { this->NumberOfSteps = numSteps; }
  void SetSeeds(const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->ClearParticles();

    vtkm::Id n = seeds.GetNumberOfValues();
    auto portal = seeds.ReadPortal();
    std::cout << "SetSeeds: n= " << n << std::endl;

    std::vector<std::vector<vtkm::Id>> blockIDs;
    std::vector<ParticleType> particles;
    for (vtkm::Id i = 0; i < n; i++)
    {
      const ParticleType p = portal.Get(i);
      std::vector<vtkm::Id> ids = this->BoundsMap.FindBlocks(p.Pos);
      std::cout << "  " << i << " " << p.Pos << " ids= " << ids.size()
                << " BM= " << this->BoundsMap.GlobalBounds << " "
                << this->BoundsMap.FindRank(ids[0]) << " " << this->Rank << std::endl;

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
    vtkm::filter::particleadvection::ParticleMessenger<ParticleType> messenger(
      this->Comm, this->BoundsMap, 1, 128);

    vtkm::Id nLocal = static_cast<vtkm::Id>(this->Active.size() + this->Inactive.size());
    this->ComputeTotalNumParticles(nLocal);
    this->TotalNumTerminatedParticles = 0;
    std::cout << "Go() nParticles= " << nLocal << std::endl;
    std::cout << "    BM= " << this->BoundsMap.GlobalBounds << std::endl;

    while (this->TotalNumTerminatedParticles < this->TotalNumParticles)
    {
      std::vector<ParticleType> v;
      vtkm::Id numTerm = 0, blockId = -1;
      if (this->GetActiveParticles(v, blockId))
      {
        //make this a pointer to avoid the copy?
        auto block = this->GetDataSet(blockId);
        DSIStuff<ParticleType> stuff(this->BoundsMap, this->ParticleBlockIDsMap);
        block->Advect(v, this->StepSize, this->NumberOfSteps, stuff);
        numTerm = this->UpdateResult(stuff);
        std::cout << " Advect: " << v.size() << " NT= " << numTerm << std::endl;
      }

      vtkm::Id numTermMessages = 0;
      this->Communicate(messenger, numTerm, numTermMessages);

      this->TotalNumTerminatedParticles += (numTerm + numTermMessages);
      if (this->TotalNumTerminatedParticles > this->TotalNumParticles)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");
    }
  }


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

  DSI* GetDataSet(vtkm::Id id)
  {
    for (auto& it : this->Blocks)
      if (it->GetID() == id)
        return it;

    throw vtkm::cont::ErrorFilterExecution("Bad block");
  }

  virtual void SetSeedArray(const std::vector<ParticleType>& particles,
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
    std::cout << " numActive= " << this->Active.size() << " p= " << particles.size() << std::endl;
  }

  virtual bool GetActiveParticles(std::vector<ParticleType>& particles, vtkm::Id& blockId)
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

  virtual void Communicate(
    vtkm::filter::particleadvection::ParticleMessenger<ParticleType>& messenger,
    vtkm::Id numLocalTerminations,
    vtkm::Id& numTermMessages)
  {
    std::vector<ParticleType> incoming;
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

  virtual void UpdateActive(const std::vector<ParticleType>& particles,
                            const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    this->Update(this->Active, particles, idsMap);
  }

  virtual void UpdateInactive(const std::vector<ParticleType>& particles,
                              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    this->Update(this->Inactive, particles, idsMap);
  }

  void Update(std::vector<ParticleType>& arr,
              const std::vector<ParticleType>& particles,
              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    VTKM_ASSERT(particles.size() == idsMap.size());

    arr.insert(arr.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  /*
  void UpdateTerminated(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                        const std::vector<vtkm::Id>& idxTerm)
  {
    auto portal = particles.ReadPortal();
    for (const auto& idx : idxTerm)
      this->ParticleBlockIDsMap.erase(portal.Get(idx).ID);
  }
  */

  /*
  void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                         std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapA,
                         std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapI,
                         std::vector<ParticleType>& A,
                         std::vector<ParticleType>& I,
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
  */

  vtkm::Id UpdateResult(const DSIStuff<ParticleType>& stuff)
  {
    this->UpdateActive(stuff.A, stuff.IdMapA);
    this->UpdateInactive(stuff.I, stuff.IdMapI);

    vtkm::Id numTerm = static_cast<vtkm::Id>(stuff.TermID.size());
    //Update terminated particles.
    if (numTerm > 0)
    {
      for (const auto& id : stuff.TermID)
        this->ParticleBlockIDsMap.erase(id);
    }

    return numTerm;
  }

  /*
  vtkm::Id UpdateResult(ResultType<ParticleType>& res, vtkm::Id blockId)
  {
    std::cout<<"UpdateResult: blockId= "<<blockId<<std::endl;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> idsMapI, idsMapA;
    std::vector<ParticleType> A, I;
    std::vector<vtkm::Id> termIdx;
    this->ClassifyParticles(res.Particles, idsMapA, idsMapI, A, I, termIdx);

    //Update active, inactive and terminated
    this->UpdateActive(A, idsMapA);
    this->UpdateInactive(I, idsMapI);

    vtkm::Id numTerm = static_cast<vtkm::Id>(termIdx.size());
    if (numTerm > 0)
      this->UpdateTerminated(res.Particles, termIdx);

    internal::ResultHelper<ResultType, ParticleType>::Store(this->Results, res, blockId, termIdx);

    return numTerm;
  }
  */

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
  std::vector<ParticleType> Active;
  std::vector<DSIType*> Blocks;
  vtkm::filter::particleadvection::BoundsMap BoundsMap;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  std::vector<ParticleType> Inactive;
  vtkm::Id NumberOfSteps;
  vtkm::Id NumRanks;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
  vtkm::Id Rank;
  std::map<vtkm::Id, std::vector<ResultType<ParticleType>>> Results;
  vtkm::FloatDefault StepSize;
  vtkm::Id TotalNumParticles;
  vtkm::Id TotalNumTerminatedParticles;
};

}
}
}

//#include <vtkm/filter/particleadvection/ABA.hxx>

#endif //vtk_m_filter_ABA_h
