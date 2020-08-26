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
ParticleAdvectorBase<ResultType>::GetDataSet(vtkm::Id id) const
{
  for (const auto& it : this->Blocks)
    if (it.GetID() == id)
      return it;
  throw vtkm::cont::ErrorFilterExecution("Bad block");
}

template <typename ResultType>
inline void ParticleAdvectorBase<ResultType>::UpdateResult(const ResultType& res,
                                                           vtkm::Id blockId,
                                                           std::vector<vtkm::Particle>& I,
                                                           std::vector<vtkm::Particle>& T,
                                                           std::vector<vtkm::Particle>& A)
{
  vtkm::Id n = res.Particles.GetNumberOfValues();
  auto portal = res.Particles.ReadPortal();

  for (vtkm::Id i = 0; i < n; i++)
  {
    vtkm::Particle p = portal.Get(i);

    if (p.Status.CheckTerminate())
    {
      T.push_back(p);
      this->ParticleBlockIDsMap.erase(p.ID);
    }
    else
    {
      std::vector<vtkm::Id> currBIDs = this->ParticleBlockIDsMap[p.ID];
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
        T.push_back(p);
        this->ParticleBlockIDsMap.erase(p.ID);
      }
      else
      {
        //If we have more than blockId, we want to minimize communication
        //and put any blocks owned by this rank first.
        if (newIDs.size() > 1)
        {
          for (auto it = newIDs.begin(); it != newIDs.end(); it++)
          {
            vtkm::Id bid = *it;
            if (this->BoundsMap.FindRank(bid) == this->Rank)
            {
              newIDs.erase(it);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        int dstRank = this->BoundsMap.FindRank(newIDs[0]);
        this->ParticleBlockIDsMap[p.ID] = newIDs;

        if (dstRank == this->Rank)
          A.push_back(p);
        else
          I.push_back(p);
      }
    }
  }
  this->StoreResult(res, blockId);
}
}
}
} // namespace vtkm::filter::particleadvection

#endif
