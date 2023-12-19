//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocatorTwoLevel_h
#define vtk_m_cont_CellLocatorTwoLevel_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetList.h>

#include <vtkm/cont/internal/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorMultiplexer.h>
#include <vtkm/exec/CellLocatorTwoLevel.h>


namespace vtkm
{
namespace cont
{

/// \brief A locator that uses 2 nested levels of grids.
///
/// `CellLocatorTwoLevel` creates a cell search structure using two levels of structured
///  grids. The first level is a coarse grid that covers the entire region of the data.
/// It is expected that the distributions of dataset cells in this coarse grid will be
/// very uneven. Within each bin of the coarse grid, a second level grid is defined within
/// the spatial bounds of the coarse bin. The size of this second level grid is proportional
/// to the number of cells in the first level. In this way, the second level grids adapt
/// to the distribution of cells being located. The adaption is not perfect, but it is
/// has very good space efficiency and is fast to generate and use.
///
/// The algorithm used in `CellLocatorTwoLevel` is described in the following publication:
///
/// Javor Kalojanov, Markus Billeter, and Philipp Slusallek. "Two-Level Grids for Ray Tracing
/// on GPUs." _Computer Graphics Forum_, 2011, pages 307-314. DOI 10.1111/j.1467-8659.2011.01862.x
///
class VTKM_CONT_EXPORT CellLocatorTwoLevel
  : public vtkm::cont::internal::CellLocatorBase<CellLocatorTwoLevel>
{
  using Superclass = vtkm::cont::internal::CellLocatorBase<CellLocatorTwoLevel>;

  template <typename CellSetCont>
  using CellSetContToExec =
    typename CellSetCont::template ExecConnectivityType<vtkm::TopologyElementTagCell,
                                                        vtkm::TopologyElementTagPoint>;

public:
  using SupportedCellSets = VTKM_DEFAULT_CELL_SET_LIST;

  using CellExecObjectList = vtkm::ListTransform<SupportedCellSets, CellSetContToExec>;
  using CellLocatorExecList =
    vtkm::ListTransform<CellExecObjectList, vtkm::exec::CellLocatorTwoLevel>;

  using ExecObjType = vtkm::ListApply<CellLocatorExecList, vtkm::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  CellLocatorTwoLevel()
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

  ExecObjType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                  vtkm::cont::Token& token) const;

private:
  friend Superclass;
  VTKM_CONT void Build();

  vtkm::FloatDefault DensityL1, DensityL2;

  vtkm::internal::cl_uniform_bins::Grid TopLevel;
  vtkm::cont::ArrayHandle<vtkm::internal::cl_uniform_bins::DimVec3> LeafDimensions;
  vtkm::cont::ArrayHandle<vtkm::Id> LeafStartIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> CellStartIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> CellCount;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIds;

  struct MakeExecObject;
};

}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorTwoLevel_h
