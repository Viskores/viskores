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
#include <vtkm/cont/CellSetList.h>

#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorMultiplexer.h>
#include <vtkm/exec/CellLocatorUniformBins.h>


namespace vtkm
{
namespace cont
{

/// \brief A locator that uses a uniform grid
///
/// `CellLocatorUniformBins` creates a cell search structure using a single uniform
/// grid. The size of the uniform grid is specified using the `SetDims` method.
/// In general, the `CellLocatorTwoLevel` has the better performance. However,
/// there are some cases where this is not the case. One example of this is
/// a uniformly dense triangle grid. In some cases the `CellLocatorUniformBins`
/// produces a more efficient search structure, especially for GPUs where memory
/// access patterns are critical to performance.
class VTKM_CONT_EXPORT CellLocatorUniformBins : public vtkm::cont::CellLocatorBase
{
  template <typename CellSetCont>
  using CellSetContToExec =
    typename CellSetCont::template ExecConnectivityType<vtkm::TopologyElementTagCell,
                                                        vtkm::TopologyElementTagPoint>;

public:
  using SupportedCellSets = VTKM_DEFAULT_CELL_SET_LIST;

  using CellExecObjectList = vtkm::ListTransform<SupportedCellSets, CellSetContToExec>;
  using CellLocatorExecList =
    vtkm::ListTransform<CellExecObjectList, vtkm::exec::CellLocatorUniformBins>;

  using ExecObjType = vtkm::ListApply<CellLocatorExecList, vtkm::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  CellLocatorUniformBins() = default;

  /// @brief Specify the dimensions of the grid used to establish bins.
  ///
  /// This locator will establish a grid over the bounds of the input data
  /// that contains the number of bins specified by these dimensions in each
  /// direction. Larger dimensions will reduce the number of cells in each bin,
  /// but will require more memory. `SetDims()` must be called before `Update()`.
  VTKM_CONT void SetDims(const vtkm::Id3& dims) { this->UniformDims = dims; }
  /// @copydoc SetDims
  VTKM_CONT vtkm::Id3 GetDims() const { return this->UniformDims; }

  /// Print a summary of the state of this locator.
  void PrintSummary(std::ostream& out) const;

public:
  ExecObjType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                  vtkm::cont::Token& token) const;

private:
  VTKM_CONT void Build() override;

  vtkm::Vec3f InvSpacing;
  vtkm::Vec3f MaxPoint;
  vtkm::Vec3f Origin;
  vtkm::Id3 UniformDims;
  vtkm::Id3 MaxCellIds;

  using CellIdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using CellIdOffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;

  vtkm::cont::ArrayHandleGroupVecVariable<CellIdArrayType, CellIdOffsetArrayType> CellIds;

  struct MakeExecObject;
};

}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorUniformBins_h
