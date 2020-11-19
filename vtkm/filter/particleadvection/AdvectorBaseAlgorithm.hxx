//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_hxx
#define vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

using PAResultType = vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>;
using SLResultType = vtkm::worklet::StreamlineResult<vtkm::Particle>;

template <>
inline void AdvectorBaseAlgorithm<PAResultType>::StoreResult(const PAResultType& vtkmNotUsed(res),
                                                             vtkm::Id vtkmNotUsed(blockId))
{
}

template <>
inline void AdvectorBaseAlgorithm<SLResultType>::StoreResult(const SLResultType& res,
                                                             vtkm::Id blockId)
{
  this->Results[blockId].push_back(res);
}

template <>
inline vtkm::cont::PartitionedDataSet AdvectorBaseAlgorithm<PAResultType>::GetOutput()
{
  vtkm::cont::PartitionedDataSet output;

  for (const auto& it : this->Terminated)
  {
    if (it.second.empty())
      continue;

    auto particles = vtkm::cont::make_ArrayHandle(it.second, vtkm::CopyFlag::Off);
    vtkm::cont::ArrayHandle<vtkm::Vec3f> pos;
    vtkm::cont::ParticleArrayCopy(particles, pos);

    vtkm::cont::DataSet ds;
    vtkm::cont::CoordinateSystem outCoords("coordinates", pos);
    ds.AddCoordinateSystem(outCoords);

    //Create vertex cell set
    vtkm::Id numPoints = pos.GetNumberOfValues();
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

template <>
inline vtkm::cont::PartitionedDataSet AdvectorBaseAlgorithm<SLResultType>::GetOutput()
{
  vtkm::cont::PartitionedDataSet output;

  for (const auto& it : this->Results)
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
      auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numPointsPerCellArray);

      vtkm::cont::CellSetExplicit<> polyLines;
      polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
      ds.SetCellSet(polyLines);
    }
    output.AppendPartition(ds);
  }
  return output;
}

template <typename ResultType>
inline vtkm::Id AdvectorBaseAlgorithm<ResultType>::ComputeTotalNumParticles(vtkm::Id numLocal) const
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
AdvectorBaseAlgorithm<ResultType>::GetDataSet(vtkm::Id id) const
{
  for (const auto& it : this->Blocks)
    if (it.GetID() == id)
      return it;
  throw vtkm::cont::ErrorFilterExecution("Bad block");
}

template <typename ResultType>
inline void AdvectorBaseAlgorithm<ResultType>::UpdateResultParticle(
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
inline vtkm::Id AdvectorBaseAlgorithm<ResultType>::UpdateResult(const ResultType& res,
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

#endif //vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_hxx
