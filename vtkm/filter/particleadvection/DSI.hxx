//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DSI_hxx
#define vtk_m_filter_particleadvection_DSI_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename ParticleType>
VTKM_CONT void DSI::ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                      DSIHelperInfo<ParticleType>& stuff) const
{
  stuff.A.clear();
  stuff.I.clear();
  stuff.TermID.clear();
  stuff.TermIdx.clear();
  stuff.IdMapI.clear();
  stuff.IdMapA.clear();

  auto portal = particles.WritePortal();
  vtkm::Id n = portal.GetNumberOfValues();

  for (vtkm::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);

    if (p.Status.CheckTerminate())
    {
      stuff.TermIdx.push_back(i);
      stuff.TermID.push_back(p.ID);
    }
    else
    {
      const auto& it = stuff.ParticleBlockIDsMap.find(p.ID);
      VTKM_ASSERT(it != stuff.ParticleBlockIDsMap.end());
      auto currBIDs = it->second;
      VTKM_ASSERT(!currBIDs.empty());

      std::vector<vtkm::Id> newIDs;
      if (p.Status.CheckSpatialBounds() && !p.Status.CheckTookAnySteps())
        newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
      else
        newIDs = stuff.BoundsMap.FindBlocks(p.Pos, currBIDs);

      //reset the particle status.
      p.Status = vtkm::ParticleStatus();

      if (newIDs.empty()) //No blocks, we're done.
      {
        p.Status.SetTerminate();
        stuff.TermIdx.push_back(i);
        stuff.TermID.push_back(p.ID);
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
            if (stuff.BoundsMap.FindRank(bid) == this->Rank)
            {
              newIDs.erase(idit);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        int dstRank = stuff.BoundsMap.FindRank(newIDs[0]);
        if (dstRank == this->Rank)
        {
          stuff.A.push_back(p);
          stuff.IdMapA[p.ID] = newIDs;
        }
        else
        {
          stuff.I.push_back(p);
          stuff.IdMapI[p.ID] = newIDs;
        }
      }
      portal.Set(i, p);
    }
  }

  //Make sure we didn't miss anything. Every particle goes into a single bucket.
  VTKM_ASSERT(static_cast<std::size_t>(n) ==
              (stuff.A.size() + stuff.I.size() + stuff.TermIdx.size()));
  VTKM_ASSERT(stuff.TermIdx.size() == stuff.TermID.size());
}

template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT void DSI::UpdateResult(const ResultType<ParticleType>& result,
                                 DSIHelperInfo<ParticleType>& stuff)
{
  this->ClassifyParticles(result.Particles, stuff);

  //template this for PA and SL
  if (this->ResType ==
      vtkm::filter::particleadvection::ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
    if (stuff.TermIdx.empty())
      return;

    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;
    auto indicesAH = vtkm::cont::make_ArrayHandle(stuff.TermIdx, vtkm::CopyFlag::Off);
    auto termPerm = vtkm::cont::make_ArrayHandlePermutation(indicesAH, result.Particles);

    vtkm::cont::ArrayHandle<ParticleType> termParticles;
    vtkm::cont::Algorithm::Copy(termPerm, termParticles);

    ResType termRes(termParticles);
    this->Results.push_back(termRes);
  }
  else if (this->ResType ==
           vtkm::filter::particleadvection::ParticleAdvectionResultType::STREAMLINE_TYPE)
  {
    this->Results.push_back(result);
  }
}

template <typename ParticleType>
VTKM_CONT bool DSI::GetOutput(vtkm::cont::DataSet& ds) const
{
  std::size_t nResults = this->Results.size();
  if (nResults == 0)
    return false;

  std::cout << "Check the variant type!" << std::endl;

  if (this->ResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;

    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);
    for (const auto& vres : this->Results)
      allParticles.push_back(vres.Get<ResType>().Particles);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
    vtkm::cont::ParticleArrayCopy(allParticles, pts);
    std::cout << "DSI::GetOutput() pts= ";
    vtkm::cont::printSummary_ArrayHandle(pts, std::cout);

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
  else if (this->ResType == ParticleAdvectionResultType::STREAMLINE_TYPE)
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

#endif //vtk_m_filter_particleadvection_DSI_hxx
