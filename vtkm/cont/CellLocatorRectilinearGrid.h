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

#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/VirtualObjectHandle.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorRectilinearGrid : public vtkm::cont::CellLocator
{
public:
  VTKM_CONT CellLocatorRectilinearGrid();

  VTKM_CONT ~CellLocatorRectilinearGrid() override;

  VTKM_CONT const vtkm::exec::CellLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const override;

protected:
  VTKM_CONT void Build() override;

private:
  vtkm::Bounds Bounds;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  bool Is3D = true;

  mutable vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator> ExecutionObjectHandle;
};

} //namespace cont
} //namespace vtkm

#endif //vtkm_cont_CellLocatorRectilinearGrid_h
