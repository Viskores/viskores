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

  vtkm::filter::flow::internal::BoundsMap BoundsMap;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;

  ParticleBlockIds InBounds;
  ParticleBlockIds OutOfBounds;
  std::vector<ParticleType> Particles;
  std::vector<vtkm::Id> TermID;
  std::vector<vtkm::Id> TermIdx;
};

template <typename Derived, typename ParticleType>
class DataSetIntegrator
{
public:
  DataSetIntegrator(vtkm::Id id, vtkm::filter::flow::IntegrationSolverType solverType)
    : Id(id)
    , SolverType(solverType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  VTKM_CONT
  void Advect(DSIHelperInfo<ParticleType>& b,
              vtkm::FloatDefault stepSize) //move these to member data(?)
  {
    Derived* inst = static_cast<Derived*>(this);
    inst->DoAdvect(b, stepSize);
  }

  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& dataset) const
  {
    Derived* inst = static_cast<Derived*>(this);
    return inst->GetOutput(dataset);
  }

protected:
  VTKM_CONT inline void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                          DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  vtkm::Id Id;
  vtkm::filter::flow::IntegrationSolverType SolverType;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;
  bool CopySeedArray = false;
};

template <typename Derived, typename ParticleType>
VTKM_CONT inline void DataSetIntegrator<Derived, ParticleType>::ClassifyParticles(
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

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegrator_h
