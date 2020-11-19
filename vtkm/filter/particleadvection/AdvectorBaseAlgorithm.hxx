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

//Result specific implementation.
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

}
}
} // namespace vtkm::filter::particleadvection

#endif //vtk_m_filter_particleadvection_AdvectorBaseAlgorithm_hxx
