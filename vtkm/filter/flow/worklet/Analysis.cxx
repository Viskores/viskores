//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================
#define vtk_m_filter_flow_worklet_Analysis_cxx

#include <vtkm/filter/flow/worklet/Analysis.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
VTKM_CONT bool NoAnalysis<ParticleType>::MakeDataSet(
  vtkm::cont::DataSet& dataset,
  const std::vector<NoAnalysis<ParticleType>>& results)
{
  size_t nResults = results.size();
  std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
  allParticles.reserve(nResults);
  for (const auto& vres : results)
    allParticles.emplace_back(vres.Particles);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
  vtkm::cont::ParticleArrayCopy(allParticles, pts);

  vtkm::Id numPoints = pts.GetNumberOfValues();
  if (numPoints > 0)
  {
    //Create coordinate system and vertex cell set.
    dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", pts));

    vtkm::cont::CellSetSingleType<> cells;
    vtkm::cont::ArrayHandleIndex conn(numPoints);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

    vtkm::cont::ArrayCopy(conn, connectivity);
    cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
    dataset.SetCellSet(cells);
  }
  return true;
}

namespace detail
{
class GetSteps : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  GetSteps() {}
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ParticleType>
  VTKM_EXEC void operator()(const ParticleType& p, vtkm::Id& numSteps) const
  {
    numSteps = p.GetNumberOfSteps();
  }
};

class ComputeNumPoints : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  ComputeNumPoints() {}
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  // Offset is number of points in streamline.
  // 1 (inital point) + number of steps taken (p.NumSteps - initalNumSteps)
  template <typename ParticleType>
  VTKM_EXEC void operator()(const ParticleType& p,
                            const vtkm::Id& initialNumSteps,
                            vtkm::Id& diff) const
  {
    diff = 1 + p.GetNumberOfSteps() - initialNumSteps;
  }
};

} // namespace detail

template <typename ParticleType>
VTKM_CONT void StreamlineAnalysis<ParticleType>::InitializeAnalysis(
  const vtkm::cont::ArrayHandle<ParticleType>& particles)
{
  this->NumParticles = particles.GetNumberOfValues();

  //Create ValidPointArray initialized to zero.
  vtkm::cont::ArrayHandleConstant<vtkm::Id> validity(0, this->NumParticles * (this->MaxSteps + 1));
  vtkm::cont::ArrayCopy(validity, this->Validity);
  //Create StepCountArray initialized to zero.
  vtkm::cont::ArrayHandleConstant<vtkm::Id> streamLengths(0, this->NumParticles);
  vtkm::cont::ArrayCopy(streamLengths, this->StreamLengths);

  // Initialize InitLengths
  vtkm::Id numSeeds = static_cast<vtkm::Id>(particles.GetNumberOfValues());
  vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
  vtkm::cont::Invoker invoker;
  invoker(detail::GetSteps{}, particles, this->InitialLengths);
}

template <typename ParticleType>
VTKM_CONT void StreamlineAnalysis<ParticleType>::FinalizeAnalysis(
  vtkm::cont::ArrayHandle<ParticleType>& particles)
{
  vtkm::Id numSeeds = particles.GetNumberOfValues();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> positions;
  vtkm::cont::Algorithm::CopyIf(this->Streams, this->Validity, positions, IsOne());
  vtkm::cont::Algorithm::Copy(positions, this->Streams);

  // Create the cells
  vtkm::cont::ArrayHandle<vtkm::Id> numPoints;
  vtkm::cont::Invoker invoker;
  invoker(detail::ComputeNumPoints{}, particles, this->InitialLengths, numPoints);

  vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
  vtkm::Id connectivityLen = vtkm::cont::Algorithm::ScanExclusive(numPoints, cellIndex);
  vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  vtkm::cont::ArrayCopy(connCount, connectivity);

  vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
  auto polyLineShape =
    vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_POLY_LINE, numSeeds);
  vtkm::cont::ArrayCopy(polyLineShape, cellTypes);

  auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numPoints);

  this->PolyLines.Fill(this->Streams.GetNumberOfValues(), cellTypes, connectivity, offsets);
  this->Particles = particles;
}

template <typename ParticleType>
VTKM_CONT bool StreamlineAnalysis<ParticleType>::MakeDataSet(
  vtkm::cont::DataSet& dataset,
  const std::vector<StreamlineAnalysis<ParticleType>>& results)
{
  size_t nResults = results.size();
  if (nResults == 1)
  {
    const auto& res = results[0];
    dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Streams));
    dataset.SetCellSet(res.PolyLines);
  }
  else
  {
    std::vector<vtkm::Id> posOffsets(nResults, 0);
    vtkm::Id totalNumCells = 0, totalNumPts = 0;
    for (std::size_t i = 0; i < nResults; i++)
    {
      const auto& res = results[i];
      if (i == 0)
        posOffsets[i] = 0;
      else
        posOffsets[i] = totalNumPts;

      totalNumPts += res.Streams.GetNumberOfValues();
      totalNumCells += res.PolyLines.GetNumberOfCells();
    }

    //Append all the points together.
    vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
    appendPts.Allocate(totalNumPts);
    for (std::size_t i = 0; i < nResults; i++)
    {
      const auto& res = results[i];
      // copy all values into appendPts starting at offset.
      vtkm::cont::Algorithm::CopySubRange(
        res.Streams, 0, res.Streams.GetNumberOfValues(), appendPts, posOffsets[i]);
    }
    dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", appendPts));

    //Create polylines.
    std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
    std::size_t off = 0;
    for (std::size_t i = 0; i < nResults; i++)
    {
      const auto& res = results[i];
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
    auto polyLineShape =
      vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_POLY_LINE, totalNumCells);
    vtkm::cont::ArrayCopy(polyLineShape, cellTypes);
    auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numPointsPerCellArray);

    vtkm::cont::CellSetExplicit<> polyLines;
    polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
    dataset.SetCellSet(polyLines);
  }
  return true;
}

template class VTKM_FILTER_FLOW_EXPORT NoAnalysis<vtkm::Particle>;
template class VTKM_FILTER_FLOW_EXPORT NoAnalysis<vtkm::ChargedParticle>;
template class VTKM_FILTER_FLOW_EXPORT StreamlineAnalysis<vtkm::Particle>;
template class VTKM_FILTER_FLOW_EXPORT StreamlineAnalysis<vtkm::ChargedParticle>;

} // namespace flow
} // namespace worklet
} // namespace vtkm
