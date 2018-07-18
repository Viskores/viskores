//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellLocatorTwoLevelUniformGrid_h
#define vtk_m_cont_CellLocatorTwoLevelUniformGrid_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/exec/internal/TwoLevelUniformGridExecutionObject.h>

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace cont
{

class CellLocatorTwoLevelUniformGrid
{
public:
  CellLocatorTwoLevelUniformGrid()
    : DensityL1(32.0f)
    , DensityL2(2.0f)
  {
  }

  /// Get/Set the desired approximate number of cells per level 1 bin
  ///
  void SetDensityL1(vtkm::FloatDefault val) { this->DensityL1 = val; }
  vtkm::FloatDefault GetDensityL1() const { return this->DensityL1; }

  /// Get/Set the desired approximate number of cells per level 1 bin
  ///
  void SetDensityL2(vtkm::FloatDefault val) { this->DensityL2 = val; }
  vtkm::FloatDefault GetDensityL2() const { return this->DensityL2; }

  void SetCellSet(const vtkm::cont::DynamicCellSet& cellset) { this->CellSet = cellset; }

  const vtkm::cont::DynamicCellSet& GetCellSet() const { return this->CellSet; }

  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords) { this->Coordinates = coords; }

  const vtkm::cont::CoordinateSystem& GetCoordinates() const { return this->Coordinates; }

  void PrintSummary(std::ostream& out) const
  {
    out << "DensityL1: " << this->DensityL1 << "\n";
    out << "DensityL2: " << this->DensityL2 << "\n";
    out << "Input CellSet: \n";
    this->CellSet.PrintSummary(out);
    out << "Input Coordinates: \n";
    this->Coordinates.PrintSummary(out);
    out << "LookupStructure:\n";
    out << "  TopLevelGrid\n";
    out << "    Dimensions: " << this->LookupStructure.TopLevel.Dimensions << "\n";
    out << "    Origin: " << this->LookupStructure.TopLevel.Origin << "\n";
    out << "    BinSize: " << this->LookupStructure.TopLevel.BinSize << "\n";
    out << "  LeafDimensions:\n";
    vtkm::cont::printSummary_ArrayHandle(this->LookupStructure.LeafDimensions, out);
    out << "  LeafStartIndex:\n";
    vtkm::cont::printSummary_ArrayHandle(this->LookupStructure.LeafStartIndex, out);
    out << "  CellStartIndex:\n";
    vtkm::cont::printSummary_ArrayHandle(this->LookupStructure.CellStartIndex, out);
    out << "  CellCount:\n";
    vtkm::cont::printSummary_ArrayHandle(this->LookupStructure.CellCount, out);
    out << "  CellIds:\n";
    vtkm::cont::printSummary_ArrayHandle(this->LookupStructure.CellIds, out);
  }

private:
  using DimensionType = vtkm::Int16;
  using DimVec3 = vtkm::Vec<DimensionType, 3>;
  using FloatVec3 = vtkm::Vec<vtkm::FloatDefault, 3>;

  struct BinsBBox
  {
    DimVec3 Min;
    DimVec3 Max;

    VTKM_EXEC
    bool Empty() const
    {
      return (this->Max[0] < this->Min[0]) || (this->Max[1] < this->Min[1]) ||
        (this->Max[2] < this->Min[2]);
    }
  };

  struct Bounds
  {
    FloatVec3 Min;
    FloatVec3 Max;
  };

  VTKM_EXEC_CONT static DimVec3 ComputeGridDimension(vtkm::Id numberOfCells,
                                                     const FloatVec3& size,
                                                     vtkm::FloatDefault density)
  {
    vtkm::FloatDefault nsides = 0.0f;
    vtkm::FloatDefault volume = 1.0f;
    vtkm::FloatDefault maxside = vtkm::Max(size[0], vtkm::Max(size[1], size[2]));
    for (int i = 0; i < 3; ++i)
    {
      if (size[i] / maxside >= 1e-4f)
      {
        nsides += 1.0f;
        volume *= size[i];
      }
    }

    auto r = vtkm::Pow((static_cast<vtkm::FloatDefault>(numberOfCells) / (volume * density)),
                       1.0f / nsides);
    return vtkm::Max(DimVec3(1),
                     DimVec3(static_cast<DimensionType>(size[0] * r),
                             static_cast<DimensionType>(size[1] * r),
                             static_cast<DimensionType>(size[2] * r)));
  }

  VTKM_EXEC static vtkm::Id ComputeFlatIndex(const DimVec3& idx, const DimVec3 dim)
  {
    return idx[0] + (dim[0] * (idx[1] + (dim[1] * idx[2])));
  }

  VTKM_EXEC static vtkm::exec::twolevelgrid::Grid ComputeLeafGrid(
    const DimVec3& idx,
    const DimVec3& dim,
    const vtkm::exec::twolevelgrid::Grid& l1Grid)
  {
    return { dim,
             l1Grid.Origin + (static_cast<FloatVec3>(idx) * l1Grid.BinSize),
             l1Grid.BinSize / static_cast<FloatVec3>(dim) };
  }

  template <typename PointsVecType>
  VTKM_EXEC static Bounds ComputeCellBounds(const PointsVecType& points)
  {
    using CoordsType = typename vtkm::VecTraits<PointsVecType>::ComponentType;
    auto numPoints = vtkm::VecTraits<PointsVecType>::GetNumberOfComponents(points);

    CoordsType minp = points[0], maxp = points[0];
    for (vtkm::IdComponent i = 1; i < numPoints; ++i)
    {
      minp = vtkm::Min(minp, points[i]);
      maxp = vtkm::Max(maxp, points[i]);
    }

    return { FloatVec3(minp), FloatVec3(maxp) };
  }

  VTKM_EXEC static BinsBBox ComputeIntersectingBins(const Bounds cellBounds,
                                                    const vtkm::exec::twolevelgrid::Grid& grid)
  {
    auto minb = static_cast<DimVec3>((cellBounds.Min - grid.Origin) / grid.BinSize);
    auto maxb = static_cast<DimVec3>((cellBounds.Max - grid.Origin) / grid.BinSize);

    return { vtkm::Max(DimVec3(0), minb), vtkm::Min(grid.Dimensions - DimVec3(1), maxb) };
  }

  VTKM_EXEC static vtkm::Id GetNumberOfBins(const BinsBBox& binsBBox)
  {
    return binsBBox.Empty() ? 0 : ((binsBBox.Max[0] - binsBBox.Min[0] + 1) *
                                   (binsBBox.Max[1] - binsBBox.Min[1] + 1) *
                                   (binsBBox.Max[2] - binsBBox.Min[2] + 1));
  }

  class BBoxIterator
  {
  public:
    VTKM_EXEC_CONT BBoxIterator(const BinsBBox& bbox, const DimVec3& dim)
      : BBox(bbox)
      , Dim(dim)
      , Idx(bbox.Min)
      , StepY(dim[0] - (bbox.Max[0] - bbox.Min[0] + 1))
      , StepZ((dim[0] * dim[1]) - ((bbox.Max[1] - bbox.Min[1] + 1) * dim[0]))
      , FlatIdx(ComputeFlatIndex(this->Idx, dim))
      , DoneFlag(bbox.Empty())
    {
    }

    VTKM_EXEC_CONT void Init()
    {
      this->Idx = this->BBox.Min;
      this->FlatIdx = ComputeFlatIndex(this->Idx, this->Dim);
      this->DoneFlag = this->BBox.Empty();
    }

    VTKM_EXEC_CONT bool Done() const { return this->DoneFlag; }

    VTKM_EXEC_CONT void Next()
    {
      if (!this->DoneFlag)
      {
        ++this->Idx[0];
        this->FlatIdx += 1;
        if (this->Idx[0] > this->BBox.Max[0])
        {
          this->Idx[0] = this->BBox.Min[0];
          ++this->Idx[1];
          this->FlatIdx += this->StepY;
          if (this->Idx[1] > this->BBox.Max[1])
          {
            this->Idx[1] = this->BBox.Min[1];
            ++this->Idx[2];
            this->FlatIdx += this->StepZ;
            if (this->Idx[2] > this->BBox.Max[2])
            {
              this->DoneFlag = true;
            }
          }
        }
      }
    }

    VTKM_EXEC_CONT const DimVec3& GetIdx() const { return this->Idx; }

    VTKM_EXEC_CONT vtkm::Id GetFlatIdx() const { return this->FlatIdx; }

  private:
    BinsBBox BBox;
    DimVec3 Dim;
    DimVec3 Idx;
    vtkm::Id StepY, StepZ;
    vtkm::Id FlatIdx;
    bool DoneFlag;
  };

  // TODO: This function may return false positives for non 3D cells as the
  // tests are done on the projection of the point on the cell. Extra checks
  // should be added to test if the point actually falls on the cell.
  template <typename CellShapeTag, typename CoordsType>
  VTKM_EXEC static bool PointInsideCell(FloatVec3 point,
                                        CellShapeTag cellShape,
                                        CoordsType cellPoints,
                                        const vtkm::exec::FunctorBase& worklet,
                                        FloatVec3& parametricCoordinates)
  {
    auto bounds = ComputeCellBounds(cellPoints);
    if (point[0] >= bounds.Min[0] && point[0] <= bounds.Max[0] && point[1] >= bounds.Min[1] &&
        point[1] <= bounds.Max[1] && point[2] >= bounds.Min[2] && point[2] <= bounds.Max[2])
    {
      bool success = false;
      parametricCoordinates = vtkm::exec::WorldCoordinatesToParametricCoordinates(
        cellPoints, point, cellShape, success, worklet);
      return success && vtkm::exec::CellInside(parametricCoordinates, cellShape);
    }
    return false;
  }

public:
  class CountBinsL1 : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<Vec3> coords,
                                  FieldOutCell<IdType> bincount);
    using ExecutionSignature = void(_2, _3);

    CountBinsL1(const vtkm::exec::twolevelgrid::Grid& grid)
      : L1Grid(grid)
    {
    }

    template <typename PointsVecType>
    VTKM_EXEC void operator()(const PointsVecType& points, vtkm::Id& numBins) const
    {
      auto cellBounds = ComputeCellBounds(points);
      auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);
      numBins = GetNumberOfBins(binsBBox);
    }

  private:
    vtkm::exec::twolevelgrid::Grid L1Grid;
  };

  class FindBinsL1 : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<Vec3> coords,
                                  FieldInCell<IdType> offsets,
                                  WholeArrayOut<IdType> binIds);
    using ExecutionSignature = void(_2, _3, _4);

    FindBinsL1(const vtkm::exec::twolevelgrid::Grid& grid)
      : L1Grid(grid)
    {
    }

    template <typename PointsVecType, typename BinIdsPortalType>
    VTKM_EXEC void operator()(const PointsVecType& points,
                              vtkm::Id offset,
                              BinIdsPortalType& binIds) const
    {
      auto cellBounds = ComputeCellBounds(points);
      auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

      for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
      {
        binIds.Set(offset, i.GetFlatIdx());
        ++offset;
      }
    }

  private:
    vtkm::exec::twolevelgrid::Grid L1Grid;
  };

  class GenerateBinsL1 : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<IdType> binIds,
                                  FieldIn<IdType> cellCounts,
                                  WholeArrayOut<vtkm::ListTagBase<DimVec3>> dimensions);
    using ExecutionSignature = void(_1, _2, _3);

    using InputDomain = _1;

    GenerateBinsL1(FloatVec3 size, vtkm::FloatDefault density)
      : Size(size)
      , Density(density)
    {
    }

    template <typename OutputDimensionsPortal>
    VTKM_EXEC void operator()(vtkm::Id binId,
                              vtkm::Id numCells,
                              OutputDimensionsPortal& dimensions) const
    {
      dimensions.Set(binId, ComputeGridDimension(numCells, this->Size, this->Density));
    }

  private:
    FloatVec3 Size;
    vtkm::FloatDefault Density;
  };

  class CountBinsL2 : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<Vec3> coords,
                                  WholeArrayIn<vtkm::ListTagBase<DimVec3>> binDimensions,
                                  FieldOutCell<IdType> bincount);
    using ExecutionSignature = void(_2, _3, _4);

    CountBinsL2(const vtkm::exec::twolevelgrid::Grid& grid)
      : L1Grid(grid)
    {
    }

    template <typename PointsVecType, typename BinDimensionsPortalType>
    VTKM_EXEC void operator()(const PointsVecType& points,
                              const BinDimensionsPortalType& binDimensions,
                              vtkm::Id& numBins) const
    {
      auto cellBounds = ComputeCellBounds(points);
      auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

      numBins = 0;
      for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
      {
        vtkm::exec::twolevelgrid::Grid leaf =
          ComputeLeafGrid(i.GetIdx(), binDimensions.Get(i.GetFlatIdx()), this->L1Grid);
        auto binsBBoxL2 = ComputeIntersectingBins(cellBounds, leaf);
        numBins += GetNumberOfBins(binsBBoxL2);
      }
    }

  private:
    vtkm::exec::twolevelgrid::Grid L1Grid;
  };

  class FindBinsL2 : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<Vec3> coords,
                                  WholeArrayIn<vtkm::ListTagBase<DimVec3>> binDimensions,
                                  WholeArrayIn<IdType> binStarts,
                                  FieldInCell<IdType> offsets,
                                  WholeArrayOut<IdType> binIds,
                                  WholeArrayOut<IdType> cellIds);
    using ExecutionSignature = void(InputIndex, _2, _3, _4, _5, _6, _7);

    FindBinsL2(const vtkm::exec::twolevelgrid::Grid& grid)
      : L1Grid(grid)
    {
    }

    template <typename PointsVecType,
              typename BinDimensionsPortalType,
              typename BinStartsPortalType,
              typename BinIdsPortalType,
              typename CellIdsPortalType>
    VTKM_EXEC void operator()(vtkm::Id cellId,
                              const PointsVecType& points,
                              const BinDimensionsPortalType& binDimensions,
                              const BinStartsPortalType& binStarts,
                              vtkm::Id offset,
                              BinIdsPortalType binIds,
                              CellIdsPortalType cellIds) const
    {
      auto cellBounds = ComputeCellBounds(points);
      auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

      for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
      {
        vtkm::exec::twolevelgrid::Grid leaf =
          ComputeLeafGrid(i.GetIdx(), binDimensions.Get(i.GetFlatIdx()), this->L1Grid);
        auto binsBBoxL2 = ComputeIntersectingBins(cellBounds, leaf);
        vtkm::Id leafStart = binStarts.Get(i.GetFlatIdx());

        for (BBoxIterator j(binsBBoxL2, leaf.Dimensions); !j.Done(); j.Next())
        {
          binIds.Set(offset, leafStart + j.GetFlatIdx());
          cellIds.Set(offset, cellId);
          ++offset;
        }
      }
    }

  private:
    vtkm::exec::twolevelgrid::Grid L1Grid;
  };

  class GenerateBinsL2 : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<IdType> binIds,
                                  FieldIn<IdType> startsIn,
                                  FieldIn<IdType> countsIn,
                                  WholeArrayOut<IdType> startsOut,
                                  WholeArrayOut<IdType> countsOut);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    using InputDomain = _1;

    template <typename CellStartsPortalType, typename CellCountsPortalType>
    VTKM_EXEC void operator()(vtkm::Id binIndex,
                              vtkm::Id start,
                              vtkm::Id count,
                              CellStartsPortalType& cellStarts,
                              CellCountsPortalType& cellCounts) const
    {
      cellStarts.Set(binIndex, start);
      cellCounts.Set(binIndex, count);
    }
  };

  struct DimensionsToCount
  {
    VTKM_EXEC vtkm::Id operator()(const DimVec3& dim) const { return dim[0] * dim[1] * dim[2]; }
  };

  /// Builds the cell locator lookup structure
  ///
  template <typename DeviceAdapter, typename CellSetList = VTKM_DEFAULT_CELL_SET_LIST_TAG>
  void Build(DeviceAdapter, CellSetList cellSetTypes = CellSetList())
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    auto cellset = this->CellSet.ResetCellSetList(cellSetTypes);
    const auto& coords = this->Coordinates;
    TwoLevelUniformGrid ls;

    // 1: Compute the top level grid
    auto bounds = this->Coordinates.GetBounds();
    FloatVec3 bmin(static_cast<vtkm::FloatDefault>(bounds.X.Min),
                   static_cast<vtkm::FloatDefault>(bounds.Y.Min),
                   static_cast<vtkm::FloatDefault>(bounds.Z.Min));
    FloatVec3 bmax(static_cast<vtkm::FloatDefault>(bounds.X.Max),
                   static_cast<vtkm::FloatDefault>(bounds.Y.Max),
                   static_cast<vtkm::FloatDefault>(bounds.Z.Max));
    auto size = bmax - bmin;
    auto fudge = vtkm::Max(FloatVec3(1e-6f), size * 1e-4f);
    size += 2.0f * fudge;

    ls.TopLevel.Dimensions =
      ComputeGridDimension(cellset.GetNumberOfCells(), size, this->DensityL1);
    ls.TopLevel.Origin = bmin - fudge;
    ls.TopLevel.BinSize = size / static_cast<FloatVec3>(ls.TopLevel.Dimensions);

    // 2: For each cell, find the number of top level bins they intersect
    vtkm::cont::ArrayHandle<vtkm::Id> binCounts;
    CountBinsL1 countL1(ls.TopLevel);
    vtkm::worklet::DispatcherMapTopology<CountBinsL1, DeviceAdapter>(countL1).Invoke(
      cellset, coords, binCounts);

    // 3: Total number of unique (cell, bin) pairs (for pre-allocating arrays)
    vtkm::Id numPairsL1 = Algorithm::ScanExclusive(binCounts, binCounts);

    // 4: For each cell find the top level bins that intersect it
    vtkm::cont::ArrayHandle<vtkm::Id> binIds;
    binIds.Allocate(numPairsL1);
    FindBinsL1 findL1(ls.TopLevel);
    vtkm::worklet::DispatcherMapTopology<FindBinsL1, DeviceAdapter>(findL1).Invoke(
      cellset, coords, binCounts, binIds);
    binCounts.ReleaseResources();

    // 5: From above, find the number of cells that intersect each top level bin
    Algorithm::Sort(binIds);
    vtkm::cont::ArrayHandle<vtkm::Id> bins;
    vtkm::cont::ArrayHandle<vtkm::Id> cellsPerBin;
    Algorithm::ReduceByKey(binIds,
                           vtkm::cont::make_ArrayHandleConstant(vtkm::Id(1), numPairsL1),
                           bins,
                           cellsPerBin,
                           vtkm::Sum());
    binIds.ReleaseResources();

    // 6: Compute level-2 dimensions
    vtkm::Id numberOfBins =
      ls.TopLevel.Dimensions[0] * ls.TopLevel.Dimensions[1] * ls.TopLevel.Dimensions[2];
    Algorithm::Copy(vtkm::cont::make_ArrayHandleConstant(DimVec3(0), numberOfBins),
                    ls.LeafDimensions);
    GenerateBinsL1 generateL1(ls.TopLevel.BinSize, this->DensityL2);
    vtkm::worklet::DispatcherMapField<GenerateBinsL1, DeviceAdapter>(generateL1)
      .Invoke(bins, cellsPerBin, ls.LeafDimensions);
    bins.ReleaseResources();
    cellsPerBin.ReleaseResources();

    // 7: Compute number of level-2 bins
    vtkm::Id numberOfLeaves = Algorithm::ScanExclusive(
      vtkm::cont::make_ArrayHandleTransform(ls.LeafDimensions, DimensionsToCount()),
      ls.LeafStartIndex);


    // 8: For each cell, find the number of l2 bins they intersect
    CountBinsL2 countL2(ls.TopLevel);
    vtkm::worklet::DispatcherMapTopology<CountBinsL2, DeviceAdapter>(countL2).Invoke(
      cellset, coords, ls.LeafDimensions, binCounts);

    // 9: Total number of unique (cell, bin) pairs (for pre-allocating arrays)
    vtkm::Id numPairsL2 = Algorithm::ScanExclusive(binCounts, binCounts);

    // 10: For each cell, find the l2 bins they intersect
    binIds.Allocate(numPairsL2);
    ls.CellIds.Allocate(numPairsL2);
    FindBinsL2 findL2(ls.TopLevel);
    vtkm::worklet::DispatcherMapTopology<FindBinsL2, DeviceAdapter>(findL2).Invoke(
      cellset, coords, ls.LeafDimensions, ls.LeafStartIndex, binCounts, binIds, ls.CellIds);
    binCounts.ReleaseResources();

    // 11: From above, find the cells that each l2 bin intersects
    Algorithm::SortByKey(binIds, ls.CellIds);
    Algorithm::ReduceByKey(binIds,
                           vtkm::cont::make_ArrayHandleConstant(vtkm::Id(1), numPairsL2),
                           bins,
                           cellsPerBin,
                           vtkm::Sum());
    binIds.ReleaseResources();

    // 12: Generate the leaf bin arrays
    vtkm::cont::ArrayHandle<vtkm::Id> cellsStart;
    Algorithm::ScanExclusive(cellsPerBin, cellsStart);

    Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numberOfLeaves),
                    ls.CellStartIndex);
    Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numberOfLeaves), ls.CellCount);
    vtkm::worklet::DispatcherMapField<GenerateBinsL2, DeviceAdapter>().Invoke(
      bins, cellsStart, cellsPerBin, ls.CellStartIndex, ls.CellCount);

    std::swap(this->LookupStructure, ls);
  }

  struct TwoLevelUniformGrid : public vtkm::cont::ExecutionObjectBase
  {
    template <typename DeviceAdapter>
    VTKM_CONT vtkm::exec::twolevelgrid::TwoLevelUniformGridExecutionObject<DeviceAdapter>
    PrepareForExecution(DeviceAdapter device) const
    {
      vtkm::exec::twolevelgrid::TwoLevelUniformGridExecutionObject<DeviceAdapter> deviceObject;
      deviceObject.TopLevel = this->TopLevel;
      deviceObject.LeafDimensions = this->LeafDimensions.PrepareForInput(device);
      deviceObject.LeafStartIndex = this->LeafStartIndex.PrepareForInput(device);
      deviceObject.CellStartIndex = this->CellStartIndex.PrepareForInput(device);
      deviceObject.CellCount = this->CellCount.PrepareForInput(device);
      deviceObject.CellIds = this->CellIds.PrepareForInput(device);
      return deviceObject;
    }

    vtkm::exec::twolevelgrid::Grid TopLevel;

    vtkm::cont::ArrayHandle<DimVec3> LeafDimensions;
    vtkm::cont::ArrayHandle<vtkm::Id> LeafStartIndex;

    vtkm::cont::ArrayHandle<vtkm::Id> CellStartIndex;
    vtkm::cont::ArrayHandle<vtkm::Id> CellCount;
    vtkm::cont::ArrayHandle<vtkm::Id> CellIds;
  };

  class FindCellWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<Vec3> points,
                                  WholeCellSetIn<> cellSet,
                                  WholeArrayIn<Vec3> coordinates,
                                  ExecObject lookupStruct,
                                  FieldOut<IdType> cellIds,
                                  FieldOut<Vec3> parametricCoordinates);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

    using InputDomain = _1;

    template <typename PointType,
              typename CellSetType,
              typename CoordsPortalType,
              typename LookupStructureType>
    VTKM_EXEC void operator()(const PointType& point,
                              const CellSetType& cellSet,
                              const CoordsPortalType& coords,
                              const LookupStructureType& lookupStruct,
                              vtkm::Id& cellId,
                              FloatVec3& parametricCoordinates) const
    {
      cellId = -1;
      FloatVec3 p(static_cast<FloatDefault>(point[0]),
                  static_cast<FloatDefault>(point[1]),
                  static_cast<FloatDefault>(point[2]));

      const vtkm::exec::twolevelgrid::Grid& topLevelGrid = lookupStruct.TopLevel;

      DimVec3 binId3 = static_cast<DimVec3>((p - topLevelGrid.Origin) / topLevelGrid.BinSize);
      if (binId3[0] >= 0 && binId3[0] < topLevelGrid.Dimensions[0] && binId3[1] >= 0 &&
          binId3[1] < topLevelGrid.Dimensions[1] && binId3[2] >= 0 &&
          binId3[2] < topLevelGrid.Dimensions[2])
      {
        vtkm::Id binId = ComputeFlatIndex(binId3, topLevelGrid.Dimensions);

        auto ldim = lookupStruct.LeafDimensions.Get(binId);
        if (!ldim[0] || !ldim[1] || !ldim[2])
        {
          return;
        }

        auto leafGrid = ComputeLeafGrid(binId3, ldim, topLevelGrid);

        DimVec3 leafId3 = static_cast<DimVec3>((p - leafGrid.Origin) / leafGrid.BinSize);
        // precision issues may cause leafId3 to be out of range so clamp it
        leafId3 = vtkm::Max(DimVec3(0), vtkm::Min(ldim - DimVec3(1), leafId3));

        vtkm::Id leafStart = lookupStruct.LeafStartIndex.Get(binId);
        vtkm::Id leafId = leafStart + ComputeFlatIndex(leafId3, leafGrid.Dimensions);

        vtkm::Id start = lookupStruct.CellStartIndex.Get(leafId);
        vtkm::Id end = start + lookupStruct.CellCount.Get(leafId);
        for (vtkm::Id i = start; i < end; ++i)
        {
          vtkm::Id cid = lookupStruct.CellIds.Get(i);
          auto indices = cellSet.GetIndices(cid);
          vtkm::VecFromPortalPermute<decltype(indices), CoordsPortalType> pts(&indices, coords);
          FloatVec3 pc;
          if (PointInsideCell(p, cellSet.GetCellShape(cid), pts, *this, pc))
          {
            cellId = cid;
            parametricCoordinates = pc;
            break;
          }
        }
      }
    }
  };

  /// Finds the containing cells for the given array of points. Returns the cell ids
  /// in the `cellIds` arrays. If a cell could not be found due to the point being
  /// outside all the cells or due to numerical errors, the cell id is set to -1.
  /// Parametric coordinates of the point inside the cell is returned in the
  /// `parametricCoords` array.
  ///
  template <typename PointComponentType,
            typename PointStorageType,
            typename DeviceAdapter,
            typename CellSetList = VTKM_DEFAULT_CELL_SET_LIST_TAG>
  void FindCells(
    const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& points,
    vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
    vtkm::cont::ArrayHandle<FloatVec3>& parametricCoords,
    DeviceAdapter,
    CellSetList cellSetTypes = CellSetList()) const
  {
    vtkm::worklet::DispatcherMapField<FindCellWorklet, DeviceAdapter>().Invoke(
      points,
      this->CellSet.ResetCellSetList(cellSetTypes),
      this->Coordinates,
      this->LookupStructure,
      cellIds,
      parametricCoords);
  }

private:
  vtkm::FloatDefault DensityL1, DensityL2;

  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coordinates;
  TwoLevelUniformGrid LookupStructure;
};
}
}

#endif // vtk_m_cont_CellLocatorTwoLevelUniformGrid_h
