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
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DataSet.h>
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
    vtkm::Id totalNumSeeds = this->ComputeTotalNumParticles(nLocal);

    vtkm::Id N = 0;
    while (N < totalNumSeeds)
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

      N += (numTerm + numTermMessages);
      if (N > totalNumSeeds)
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

  inline vtkm::Id ComputeTotalNumParticles(vtkm::Id numLocal) const;
  inline const DataSetIntegratorType& GetDataSet(vtkm::Id id) const;
  inline void UpdateResultParticle(
    vtkm::Particle& p,
    std::vector<vtkm::Particle>& I,
    std::vector<vtkm::Particle>& T,
    std::vector<vtkm::Particle>& A,
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapI,
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapA) const;

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
                       numTermMessages);
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

  inline void StoreResult(const ResultType& res, vtkm::Id blockId);
  inline vtkm::Id UpdateResult(const ResultType& res, vtkm::Id blockId);

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
  std::unordered_map<vtkm::Id, std::vector<vtkm::Particle>> Terminated;
};

}
}
} // namespace vtkm::filter::particleadvection


#ifndef vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_hxx
#include <vtkm/filter/particleadvection/AdvectorBaseAlgorithm.hxx>
#endif

#endif
