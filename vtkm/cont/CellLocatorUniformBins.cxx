//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace cont
{

namespace
{

VTKM_EXEC
inline vtkm::Id ComputeFlatIndex(const vtkm::Id3& idx, const vtkm::Id3& dims)
{
  return idx[0] + (dims[0] * (idx[1] + (dims[1] * idx[2])));
}

VTKM_EXEC
inline vtkm::Id3 GetFlatIndex(const vtkm::Vec3f& pt,
                              const vtkm::Vec3f& origin,
                              const vtkm::Vec3f& invSpacing,
                              const vtkm::Id3& maxCellIds)
{
  auto temp = pt - origin;
  temp = temp * invSpacing;

  auto logicalIdx = vtkm::Min(vtkm::Id3(temp), maxCellIds);
  return logicalIdx;
}

template <typename PointsVecType>
VTKM_EXEC inline void MinMaxIndicesForCellPoints(const PointsVecType& points,
                                                 const vtkm::Vec3f& origin,
                                                 const vtkm::Vec3f& invSpacing,
                                                 const vtkm::Id3& maxCellIds,
                                                 vtkm::Id3& minIdx,
                                                 vtkm::Id3& maxIdx)
{
  auto numPoints = vtkm::VecTraits<PointsVecType>::GetNumberOfComponents(points);

  vtkm::Bounds bounds;
  for (vtkm::IdComponent i = 0; i < numPoints; ++i)
    bounds.Include(points[i]);

  //Get 8 corners of bbox.
  minIdx = GetFlatIndex(bounds.MinCorner(), origin, invSpacing, maxCellIds);
  maxIdx = GetFlatIndex(bounds.MaxCorner(), origin, invSpacing, maxCellIds);
}

class CountCellBins : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint coords, FieldOutCell bincount);
  using ExecutionSignature = void(_2, _3);
  using InputDomain = _1;

  CountCellBins(const vtkm::Vec3f& origin,
                const vtkm::Vec3f& invSpacing,
                const vtkm::Id3& maxCellIds)
    : InvSpacing(invSpacing)
    , MaxCellIds(maxCellIds)
    , Origin(origin)
  {
  }

  template <typename PointsVecType>
  VTKM_EXEC void operator()(const PointsVecType& points, vtkm::Id& numBins) const
  {
    vtkm::Id3 idx000, idx111;
    MinMaxIndicesForCellPoints(
      points, this->Origin, this->InvSpacing, this->MaxCellIds, idx000, idx111);

    //Count the number of bins for this cell
    numBins =
      (idx111[0] - idx000[0] + 1) * (idx111[1] - idx000[1] + 1) * (idx111[2] - idx000[2] + 1);
  }

private:
  vtkm::Vec3f InvSpacing;
  vtkm::Id3 MaxCellIds;
  vtkm::Vec3f Origin;
};

class RecordBinsPerCell : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                FieldInCell start,
                                WholeArrayInOut binsPerCell,
                                WholeArrayInOut cellIds,
                                AtomicArrayInOut cellCounts);
  using ExecutionSignature = void(InputIndex, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  RecordBinsPerCell(const vtkm::Vec3f& origin,
                    const vtkm::Vec3f& invSpacing,
                    const vtkm::Id3& dims,
                    const vtkm::Id3& maxCellIds)
    : Dims(dims)
    , InvSpacing(invSpacing)
    , MaxCellIds(maxCellIds)
    , Origin(origin)
  {
  }


  template <typename PointsVecType, typename ResultArrayType, typename CellCountType>
  VTKM_EXEC void operator()(const vtkm::Id& cellIdx,
                            const PointsVecType& points,
                            const vtkm::Id& start,
                            ResultArrayType& binsPerCell,
                            ResultArrayType& cellIds,
                            CellCountType cellCounts) const
  {
    vtkm::Id3 idx000, idx111;
    MinMaxIndicesForCellPoints(
      points, this->Origin, this->InvSpacing, this->MaxCellIds, idx000, idx111);

    vtkm::Id cnt = 0;
    vtkm::Id sliceStart = ComputeFlatIndex(idx000, this->Dims);
    for (vtkm::Id k = idx000[2]; k <= idx111[2]; k++)
    {
      vtkm::Id shaftStart = sliceStart;
      for (vtkm::Id j = idx000[1]; j <= idx111[1]; j++)
      {
        vtkm::Id flatIdx = shaftStart;
        for (vtkm::Id i = idx000[0]; i <= idx111[0]; i++)
        {
          // set portals and increment cnt...
          binsPerCell.Set(start + cnt, flatIdx);
          cellIds.Set(start + cnt, cellIdx);
          cellCounts.Add(flatIdx, 1);
          ++flatIdx;
          ++cnt;
        }
        shaftStart += this->Dims[0];
      }
      sliceStart += this->Dims[0] * this->Dims[1];
    }
  }

private:
  vtkm::Id3 Dims;
  vtkm::Vec3f InvSpacing;
  vtkm::Id3 MaxCellIds;
  vtkm::Vec3f Origin;
};

} //namespace detail


//----------------------------------------------------------------------------
/// Builds the cell locator lookup structure
///
VTKM_CONT void CellLocatorUniformBins::Build()
{
  if (this->UniformDims[0] <= 0 || this->UniformDims[1] <= 0 || this->UniformDims[2] <= 0)
    throw vtkm::cont::ErrorBadValue("Grid dimensions of CellLocatorUniformBins must be > 0");

  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "CellLocatorUniformBins::Build");

  this->MaxCellIds = (vtkm::Max(this->UniformDims - vtkm::Id3(1), vtkm::Id3(0)));
  vtkm::Id totalNumBins = this->UniformDims[0] * this->UniformDims[1] * this->UniformDims[2];

  vtkm::cont::Invoker invoker;

  auto cellset = this->GetCellSet();
  const auto& coords = this->GetCoordinates();

  auto bounds = coords.GetBounds();
  this->Origin = vtkm::Vec3f(bounds.MinCorner());
  this->MaxPoint = vtkm::Vec3f(bounds.MaxCorner());
  auto size = this->MaxPoint - this->Origin;
  vtkm::Vec3f spacing(
    size[0] / this->UniformDims[0], size[1] / this->UniformDims[1], size[2] / this->UniformDims[2]);

  for (vtkm::IdComponent i = 0; i < 3; i++)
  {
    if (vtkm::Abs(spacing[i]) > 0)
      this->InvSpacing[i] = 1.0f / spacing[i];
    else
      this->InvSpacing[i] = 0;
  }

  // The following example will be used in the explanation below
  // Dataset with 3 cells: c0, c1, c2
  // 2x2 uniform grid: b0, b1, b2, b3
  // Assume that the bounding box for each cell overlaps as follows:
  // c0: b0, b1, b2
  // c1: b1
  // c2: b2
  //
  // The acceleration structure is an array of cell ids that are grouped
  // by the overlapping bin. This information can be represented using
  // vtkm::cont::ArrayHandleGroupVecVariable
  // In the example above:
  // CellIds = {c0,  c0,c1,  c0,c2,  }
  //            b0    b1       b2   b3
  //
  // The algorithm runs as follows:
  //  Given a point p, find the bin (b) that contains p.
  //  Do a point-in-cell test for each cell in bin b.
  //
  // Example:  point p is in b=1
  // vtkm::cont::ArrayHandleGroupVecVariable provides the offset and number
  // cells in bin b=1. The offset is 1 and the count is 2.
  //  Do a point-in-cell test on the 2 cells that start at offset 1
  //    CellIds[ 1 + 0], which is c0
  //    CellIds[ 1 + 1], which is c1


  //Computing this involves several steps which are described below.
  //Step 1:
  // For each cell in cellSet, count the number of bins that overlap with the cell bbox.
  // For the example above
  // binCountsPerCell = {3, 1, 1}
  // cell0 overlaps with 3 bins
  // cell1 overlaps with 1 bin
  // cell2 overlaps with 1 bin
  vtkm::cont::ArrayHandle<vtkm::Id> binCountsPerCell;
  CountCellBins countCellBins(this->Origin, this->InvSpacing, this->MaxCellIds);
  invoker(countCellBins, cellset, coords, binCountsPerCell);

  //2: Compute number of unique cell/bin pairs and start indices.

  //Step 2:
  // Given the number of bins for each cell, we can compute the offset for each cell.
  // For the example above, binOffset is:
  // {0, 3, 4}
  // and the total number, num is 5
  // c0 begins at idx=0
  // c1 begins at idx=3
  // c2 begins at idx=4
  vtkm::cont::ArrayHandle<vtkm::Id> binOffset;
  auto num = vtkm::cont::Algorithm::ScanExclusive(binCountsPerCell, binOffset);

  //Step 3:
  // Now that we know the start index and numbers, we can fill an array of bin ids.
  // binsPerCell is the list of binIds for each cell. In the example above,
  // binsPerCell = {b0,b1,b2,   b2,       b2}
  //               \ cell0 /    cell1    cell2
  // We can also compute the cellIds and number of cells per bin, cellCount
  // cids      = {c0,c0,c0, c1, c2}
  // cellCount = {3, 1, 1, 0}
  // These are set using RecordBinsPerCell worklet, which does the following
  // for each cell
  //   compute cell bbox and list of overlaping bins
  //   for each overlaping bin
  //     add the bin id to binsPerCell starting at binOffset
  //     add the cell id to the CellIds starting at binOffset
  //     increment CellCount for the bin (uses an atomic for thread safety).

  vtkm::cont::ArrayHandle<vtkm::Id> binsPerCell, cids, cellCount;
  binsPerCell.AllocateAndFill(num, 0);
  cids.Allocate(num);
  cellCount.AllocateAndFill(totalNumBins, 0);
  RecordBinsPerCell recordBinsPerCell(
    this->Origin, this->InvSpacing, this->UniformDims, this->MaxCellIds);
  invoker(recordBinsPerCell, cellset, coords, binOffset, binsPerCell, cids, cellCount);

  //Step 4:
  // binsPerCell is the overlapping bins for each cell.
  // We want to sort CellIds by the bin ID.  SortByKey does this.
  vtkm::cont::Algorithm::SortByKey(binsPerCell, cids);

  // Convert the cell counts to offsets using the helper function
  // vtkm::cont::ConvertNumComponentsToOffsets, and create the
  // CellIds that are used as the acceleration structure.
  this->CellIds = vtkm::cont::make_ArrayHandleGroupVecVariable(
    cids, vtkm::cont::ConvertNumComponentsToOffsets(cellCount));
}

//----------------------------------------------------------------------------
struct CellLocatorUniformBins::MakeExecObject
{
  template <typename CellSetType>
  VTKM_CONT void operator()(const CellSetType& cellSet,
                            vtkm::cont::DeviceAdapterId device,
                            vtkm::cont::Token& token,
                            const CellLocatorUniformBins& self,
                            ExecObjType& execObject) const
  {
    using CellStructureType = CellSetContToExec<CellSetType>;

    execObject = vtkm::exec::CellLocatorUniformBins<CellStructureType>(self.UniformDims,
                                                                       self.Origin,
                                                                       self.MaxPoint,
                                                                       self.InvSpacing,
                                                                       self.MaxCellIds,
                                                                       self.CellIds,
                                                                       cellSet,
                                                                       self.GetCoordinates(),
                                                                       device,
                                                                       token);
  }
};

CellLocatorUniformBins::ExecObjType CellLocatorUniformBins::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token) const
{
  this->Update();
  ExecObjType execObject;
  vtkm::cont::CastAndCall(this->GetCellSet(), MakeExecObject{}, device, token, *this, execObject);
  return execObject;
}

//----------------------------------------------------------------------------
void CellLocatorUniformBins::PrintSummary(std::ostream& out) const
{
  out << std::endl;
  out << "CellLocatorUniformBins" << std::endl;
  out << " UniformDims: " << this->UniformDims << std::endl;
  out << " Origin: " << this->Origin << std::endl;
  out << " MaxPoint: " << this->MaxPoint << std::endl;
  out << " InvSpacing: " << this->InvSpacing << std::endl;
  out << " MaxCellIds: " << this->MaxCellIds << std::endl;

  out << "Input CellSet: \n";
  this->GetCellSet().PrintSummary(out);
  out << "Input Coordinates: \n";
  this->GetCoordinates().PrintSummary(out);
}
}
} // vtkm::cont
