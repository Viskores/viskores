//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/exec/CellLocatorRectilinearGrid.h>

namespace vtkm
{
namespace cont
{

CellLocatorRectilinearGrid::CellLocatorRectilinearGrid() = default;

CellLocatorRectilinearGrid::~CellLocatorRectilinearGrid() = default;

using StructuredType = vtkm::cont::CellSetStructured<3>;
using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using RectilinearType = vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

void CellLocatorRectilinearGrid::Build()
{
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<RectilinearType>())
    throw vtkm::cont::ErrorInternal("Coordinates are not rectilinear.");
  if (!cellSet.IsSameType(StructuredType()))
    throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");

  vtkm::Vec<vtkm::Id, 3> celldims =
    cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagCell());

  this->PlaneSize = celldims[0] * celldims[1];
  this->RowSize = celldims[0];
}

namespace
{
struct CellLocatorRectilinearGridPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename... Args>
  VTKM_CONT bool operator()(DeviceAdapter,
                            vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>& execLocator,
                            Args&&... args) const
  {
    using ExecutionType = vtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter>;
    ExecutionType* execObject = new ExecutionType(std::forward<Args>(args)..., DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};
}

const vtkm::exec::CellLocator* CellLocatorRectilinearGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  const bool success = vtkm::cont::TryExecuteOnDevice(
    device,
    CellLocatorRectilinearGridPrepareForExecutionFunctor(),
    this->ExecutionObjectHandle,
    this->PlaneSize,
    this->RowSize,
    this->GetCellSet().template Cast<StructuredType>(),
    this->GetCoordinates().GetData().template Cast<RectilinearType>());
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorRectilinearGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}
} //namespace cont
} //namespace vtkm
