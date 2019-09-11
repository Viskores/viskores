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

using Structured2DType = vtkm::cont::CellSetStructured<2>;
using Structured3DType = vtkm::cont::CellSetStructured<3>;
using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using RectilinearType = vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

void CellLocatorRectilinearGrid::Build()
{
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<RectilinearType>())
    throw vtkm::cont::ErrorBadType("Coordinates are not rectilinear type.");

  if (cellSet.IsSameType(Structured2DType()))
  {
    this->Is3D = false;
    vtkm::Vec<vtkm::Id, 2> celldims =
      cellSet.Cast<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    this->PlaneSize = celldims[0] * celldims[1];
    this->RowSize = celldims[0];
  }
  else if (cellSet.IsSameType(Structured3DType()))
  {
    this->Is3D = true;
    vtkm::Vec<vtkm::Id, 3> celldims =
      cellSet.Cast<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    this->PlaneSize = celldims[0] * celldims[1];
    this->RowSize = celldims[0];
  }
  else
  {
    throw vtkm::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }
}

namespace
{

template <vtkm::IdComponent dimensions>
struct CellLocatorRectilinearGridPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename... Args>
  VTKM_CONT bool operator()(DeviceAdapter,
                            vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>& execLocator,
                            Args&&... args) const
  {
    using ExecutionType = vtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter, dimensions>;
    ExecutionType* execObject = new ExecutionType(std::forward<Args>(args)..., DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};
}

const vtkm::exec::CellLocator* CellLocatorRectilinearGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  bool success = false;
  if (this->Is3D)
  {
    success = vtkm::cont::TryExecuteOnDevice(
      device,
      CellLocatorRectilinearGridPrepareForExecutionFunctor<3>(),
      this->ExecutionObjectHandle,
      this->PlaneSize,
      this->RowSize,
      this->GetCellSet().template Cast<Structured3DType>(),
      this->GetCoordinates().GetData().template Cast<RectilinearType>());
  }
  else
  {
    success = vtkm::cont::TryExecuteOnDevice(
      device,
      CellLocatorRectilinearGridPrepareForExecutionFunctor<2>(),
      this->ExecutionObjectHandle,
      this->PlaneSize,
      this->RowSize,
      this->GetCellSet().template Cast<Structured2DType>(),
      this->GetCoordinates().GetData().template Cast<RectilinearType>());
  }
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorRectilinearGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}
} //namespace cont
} //namespace vtkm
