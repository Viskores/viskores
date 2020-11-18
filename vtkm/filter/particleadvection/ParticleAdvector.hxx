//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ParticleAdvector_hxx
#define vtk_m_filter_ParticleAdvector_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename ResultType>
inline vtkm::Id ParticleAdvectorBase<ResultType>::ComputeTotalNumParticles(vtkm::Id numLocal) const
{
  long long totalNumParticles = static_cast<long long>(numLocal);
#ifdef VTKM_ENABLE_MPI
  MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(this->Comm.handle());
  MPI_Allreduce(MPI_IN_PLACE, &totalNumParticles, 1, MPI_LONG_LONG, MPI_SUM, mpiComm);
#endif
  return static_cast<vtkm::Id>(totalNumParticles);
}

template <typename ResultType>
inline const vtkm::filter::particleadvection::DataSetIntegrator&
ParticleAdvectorBase<ResultType>::GetDataSet(vtkm::Id id) // const
{
  for (const auto& it : this->Blocks)
    if (it.GetID() == id)
      return it;
  throw vtkm::cont::ErrorFilterExecution("Bad block");
}

template <typename ResultType>
inline void ParticleAdvectorBase<ResultType>::UpdateResultParticle(
  vtkm::Particle& p,
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

template <typename ResultType>
inline vtkm::Id ParticleAdvectorBase<ResultType>::UpdateResult(const ResultType& res,
                                                               vtkm::Id blockId)
{
  vtkm::Id n = res.Particles.GetNumberOfValues();
  auto portal = res.Particles.ReadPortal();

  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> idsMapI, idsMapA;
  std::vector<vtkm::Particle> I, T, A;

  for (vtkm::Id i = 0; i < n; i++)
  {
    //auto& p = portal.Get(i);
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
}
}
} // namespace vtkm::filter::particleadvection

#endif
