//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocatorUniformBins_h
#define vtk_m_cont_CellLocatorUniformBins_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace internal
{
namespace cl_uniform_bins
{

using DimensionType = vtkm::Int16;
using DimVec3 = vtkm::Vec<DimensionType, 3>;
using FloatVec3 = vtkm::Vec3f;

struct Grid
{
  DimVec3 Dimensions;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};

struct Bounds
{
  FloatVec3 Min;
  FloatVec3 Max;
};

VTKM_EXEC inline vtkm::Id ComputeFlatIndex(const DimVec3& idx, const DimVec3 dim)
{
  return idx[0] + (dim[0] * (idx[1] + (dim[1] * idx[2])));
}

VTKM_EXEC inline Grid ComputeLeafGrid(const DimVec3& idx, const DimVec3& dim, const Grid& l1Grid)
{
  return { dim,
           l1Grid.Origin + (static_cast<FloatVec3>(idx) * l1Grid.BinSize),
           l1Grid.BinSize / static_cast<FloatVec3>(dim) };
}

template <typename PointsVecType>
VTKM_EXEC inline Bounds ComputeCellBounds(const PointsVecType& points)
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
}
}
} // vtkm::internal::cl_uniform_bins

namespace vtkm
{
namespace exec
{

//--------------------------------------------------------------------
template <typename CellSetType, typename DeviceAdapter>
class VTKM_ALWAYS_EXPORT CellLocatorUniformBins : public vtkm::exec::CellLocator
{
private:
  using DimVec3 = vtkm::internal::cl_uniform_bins::DimVec3;
  using FloatVec3 = vtkm::internal::cl_uniform_bins::FloatVec3;

  template <typename T>
  using ArrayPortalConst =
    typename vtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapter>::PortalConst;

  using CoordsPortalType =
    decltype(vtkm::cont::ArrayHandleVirtualCoordinates{}.PrepareForInput(DeviceAdapter{}));

  using CellSetP2CExecType =
    decltype(std::declval<CellSetType>().PrepareForInput(DeviceAdapter{},
                                                         vtkm::TopologyElementTagCell{},
                                                         vtkm::TopologyElementTagPoint{}));

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
    auto bounds = vtkm::internal::cl_uniform_bins::ComputeCellBounds(cellPoints);
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
  VTKM_CONT CellLocatorUniformBins(const vtkm::internal::cl_uniform_bins::Grid& topLevelGrid,
                                   const vtkm::cont::ArrayHandle<DimVec3>& leafDimensions,
                                   const vtkm::cont::ArrayHandle<vtkm::Id>& leafStartIndex,
                                   const vtkm::cont::ArrayHandle<vtkm::Id>& cellStartIndex,
                                   const vtkm::cont::ArrayHandle<vtkm::Id>& cellCount,
                                   const vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
                                   const CellSetType& cellSet,
                                   const vtkm::cont::CoordinateSystem& coords)
    : TopLevel(topLevelGrid)
    , LeafDimensions(leafDimensions.PrepareForInput(DeviceAdapter{}))
    , LeafStartIndex(leafStartIndex.PrepareForInput(DeviceAdapter{}))
    , CellStartIndex(cellStartIndex.PrepareForInput(DeviceAdapter{}))
    , CellCount(cellCount.PrepareForInput(DeviceAdapter{}))
    , CellIds(cellIds.PrepareForInput(DeviceAdapter{}))
    , CellSet(cellSet.PrepareForInput(DeviceAdapter{},
                                      vtkm::TopologyElementTagCell{},
                                      vtkm::TopologyElementTagPoint{}))
    , Coords(coords.GetData().PrepareForInput(DeviceAdapter{}))
  {
  }

  VTKM_EXEC_CONT virtual ~CellLocatorUniformBins() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC
  void FindCell(const FloatVec3& point,
                vtkm::Id& cellId,
                FloatVec3& parametric,
                const vtkm::exec::FunctorBase& worklet) const override
  {
    using namespace vtkm::internal::cl_uniform_bins;

    cellId = -1;

    DimVec3 binId3 = static_cast<DimVec3>((point - this->TopLevel.Origin) / this->TopLevel.BinSize);
    if (binId3[0] >= 0 && binId3[0] < this->TopLevel.Dimensions[0] && binId3[1] >= 0 &&
        binId3[1] < this->TopLevel.Dimensions[1] && binId3[2] >= 0 &&
        binId3[2] < this->TopLevel.Dimensions[2])
    {
      vtkm::Id binId = ComputeFlatIndex(binId3, this->TopLevel.Dimensions);

      auto ldim = this->LeafDimensions.Get(binId);
      if (!ldim[0] || !ldim[1] || !ldim[2])
      {
        return;
      }

      auto leafGrid = ComputeLeafGrid(binId3, ldim, this->TopLevel);

      DimVec3 leafId3 = static_cast<DimVec3>((point - leafGrid.Origin) / leafGrid.BinSize);
      // precision issues may cause leafId3 to be out of range so clamp it
      leafId3 = vtkm::Max(DimVec3(0), vtkm::Min(ldim - DimVec3(1), leafId3));

      vtkm::Id leafStart = this->LeafStartIndex.Get(binId);
      vtkm::Id leafId = leafStart + ComputeFlatIndex(leafId3, leafGrid.Dimensions);

      vtkm::Id start = this->CellStartIndex.Get(leafId);
      vtkm::Id end = start + this->CellCount.Get(leafId);
      for (vtkm::Id i = start; i < end; ++i)
      {
        vtkm::Id cid = this->CellIds.Get(i);
        auto indices = this->CellSet.GetIndices(cid);
        auto pts = vtkm::make_VecFromPortalPermute(&indices, this->Coords);
        FloatVec3 pc;
        if (PointInsideCell(point, this->CellSet.GetCellShape(cid), pts, worklet, pc))
        {
          cellId = cid;
          parametric = pc;
          break;
        }
      }
    }
  }

private:
  vtkm::internal::cl_uniform_bins::Grid TopLevel;

  ArrayPortalConst<DimVec3> LeafDimensions;
  ArrayPortalConst<vtkm::Id> LeafStartIndex;

  ArrayPortalConst<vtkm::Id> CellStartIndex;
  ArrayPortalConst<vtkm::Id> CellCount;
  ArrayPortalConst<vtkm::Id> CellIds;

  CellSetP2CExecType CellSet;
  CoordsPortalType Coords;
};
}
} // vtkm::exec


namespace vtkm
{
namespace cont
{

//----------------------------------------------------------------------------
class VTKM_CONT_EXPORT CellLocatorUniformBins : public vtkm::cont::CellLocator
{
public:
  CellLocatorUniformBins()
    : DensityL1(32.0f)
    , DensityL2(2.0f)
  {
  }

  /// Get/Set the desired approximate number of cells per level 1 bin
  ///
  void SetDensityL1(vtkm::FloatDefault val)
  {
    this->DensityL1 = val;
    this->SetModified();
  }
  vtkm::FloatDefault GetDensityL1() const { return this->DensityL1; }

  /// Get/Set the desired approximate number of cells per level 1 bin
  ///
  void SetDensityL2(vtkm::FloatDefault val)
  {
    this->DensityL2 = val;
    this->SetModified();
  }
  vtkm::FloatDefault GetDensityL2() const { return this->DensityL2; }

  void PrintSummary(std::ostream& out) const;

  const vtkm::exec::CellLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const override;

private:
  VTKM_CONT void Build() override;

  vtkm::FloatDefault DensityL1, DensityL2;

  vtkm::internal::cl_uniform_bins::Grid TopLevel;
  vtkm::cont::ArrayHandle<vtkm::internal::cl_uniform_bins::DimVec3> LeafDimensions;
  vtkm::cont::ArrayHandle<vtkm::Id> LeafStartIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> CellStartIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> CellCount;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIds;

  mutable vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator> ExecutionObjectHandle;

  struct MakeExecObject;
  struct PrepareForExecutionFunctor;
};
}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorUniformBins_h
