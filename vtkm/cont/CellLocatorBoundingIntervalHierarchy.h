//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
#define vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/cont/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorBoundingIntervalHierarchy.h>
#include <vtkm/exec/CellLocatorMultiplexer.h>

namespace vtkm
{
namespace cont
{

/// @brief A cell locator that performs a recursive division of space.
///
/// `CellLocatorBoundingIntervalHierarchy` creates a search structure by recursively
/// dividing the space in which data lives.
/// It starts by choosing an axis to split and then defines a number of splitting planes
/// (set with `SetNumberOfSplittingPlanes()`).
/// These splitting planes divide the physical region into partitions, and the cells are
/// divided among these partitions.
/// The algorithm then recurses into each region and repeats the process until the regions
/// are divided to the point where the contain no more than a maximum number of cells
/// (specified with `SetMaxLeafSize()`).
class VTKM_CONT_EXPORT CellLocatorBoundingIntervalHierarchy : public vtkm::cont::CellLocatorBase
{
public:
  using SupportedCellSets = VTKM_DEFAULT_CELL_SET_LIST;

  using CellLocatorExecList =
    vtkm::ListTransform<SupportedCellSets, vtkm::exec::CellLocatorBoundingIntervalHierarchy>;

  using ExecObjType = vtkm::ListApply<CellLocatorExecList, vtkm::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  /// Construct a `CellLocatorBoundingIntervalHierarchy` while optionally specifying the
  /// number of splitting planes and number of cells in each leaf.
  VTKM_CONT
  CellLocatorBoundingIntervalHierarchy(vtkm::IdComponent numPlanes = 4,
                                       vtkm::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  /// @brief Specify the number of splitting planes to use each time a region is divided.
  ///
  /// Larger numbers of splitting planes result in a shallower tree (which is good because
  /// it means fewer memory lookups to find a cell), but too many splitting planes could lead
  /// to poorly shaped regions that inefficiently partition cells.
  ///
  /// The default value is 4.
  VTKM_CONT void SetNumberOfSplittingPlanes(vtkm::IdComponent numPlanes)
  {
    this->NumPlanes = numPlanes;
    this->SetModified();
  }
  /// @copydoc SetNumberOfSplittingPlanes
  VTKM_CONT vtkm::IdComponent GetNumberOfSplittingPlanes() { return this->NumPlanes; }

  /// @brief Specify the number of cells in each leaf.
  ///
  /// Larger numbers for the maximum leaf size result in a shallower tree (which is good
  /// because it means fewer memory lookups to find a cell), but it also means there will
  /// be more cells to check in each leaf (which is bad as checking a cell is slower
  /// than decending a tree level).
  ///
  /// The default value is 5.
  VTKM_CONT void SetMaxLeafSize(vtkm::IdComponent maxLeafSize)
  {
    this->MaxLeafSize = maxLeafSize;
    this->SetModified();
  }
  /// @copydoc SetMaxLeafSize
  VTKM_CONT vtkm::Id GetMaxLeafSize() { return this->MaxLeafSize; }

  VTKM_CONT ExecObjType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                            vtkm::cont::Token& token) const;

private:
  vtkm::IdComponent NumPlanes;
  vtkm::IdComponent MaxLeafSize;
  vtkm::cont::ArrayHandle<vtkm::exec::CellLocatorBoundingIntervalHierarchyNode> Nodes;
  vtkm::cont::ArrayHandle<vtkm::Id> ProcessedCellIds;

  VTKM_CONT void Build() override;

  struct MakeExecObject;
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
