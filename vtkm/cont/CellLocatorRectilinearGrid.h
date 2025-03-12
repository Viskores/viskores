//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_cont_CellLocatorRectilinearGrid_h
#define vtkm_cont_CellLocatorRectilinearGrid_h

#include <vtkm/cont/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorRectilinearGrid.h>

namespace vtkm
{
namespace cont
{

/// @brief A cell locator optimized for finding cells in a rectilinear grid.
///
/// This locator is optimized for structured data that has nonuniform axis-aligned spacing.
/// For this cell locator to work, it has to be given a cell set of type
/// `vtkm::cont::CellSetStructured` and a coordinate system using a
/// `vtkm::cont::ArrayHandleCartesianProduct` for its data.
class VTKM_CONT_EXPORT CellLocatorRectilinearGrid : public vtkm::cont::CellLocatorBase
{
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;
  // Might want to handle cartesian product of both Float32 and Float64.
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

public:
  CellLocatorRectilinearGrid() = default;

  ~CellLocatorRectilinearGrid() = default;

  using LastCell = vtkm::exec::CellLocatorRectilinearGrid::LastCell;

  VTKM_CONT vtkm::exec::CellLocatorRectilinearGrid PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const;

private:
  vtkm::Bounds Bounds;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  bool Is3D = true;

protected:
  VTKM_CONT void Build() override;
};

} //namespace cont
} //namespace vtkm

#endif //vtkm_cont_CellLocatorRectilinearGrid_h
