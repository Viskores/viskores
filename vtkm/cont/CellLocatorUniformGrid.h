//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_cont_CellLocatorUniformGrid_h
#define vtkm_cont_CellLocatorUniformGrid_h

#include <vtkm/cont/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorUniformGrid.h>

namespace vtkm
{
namespace cont
{

/// @brief A cell locator optimized for finding cells in a uniform grid.
///
/// This locator is optimized for structured data that has uniform axis-aligned spacing.
/// For this cell locator to work, it has to be given a cell set of type
/// `vtkm::cont::CellSetStructured` and a coordinate system using a
/// `vtkm::cont::ArrayHandleUniformPointCoordinates` for its coordinate system.
/// If the data set matches this structure, then this locator will be faster than
/// any others.
class VTKM_CONT_EXPORT CellLocatorUniformGrid : public vtkm::cont::CellLocatorBase
{
public:
  using LastCell = vtkm::exec::CellLocatorUniformGrid::LastCell;

  VTKM_CONT vtkm::exec::CellLocatorUniformGrid PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const;

private:
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  vtkm::Vec3f Origin;
  vtkm::Vec3f InvSpacing;
  vtkm::Vec3f MaxPoint;
  bool Is3D = true;

  VTKM_CONT void Build() override;
};
}
} // vtkm::cont

#endif //vtkm_cont_CellLocatorUniformGrid_h
