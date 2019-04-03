//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

struct CellLocatorRectilinearGrid::PrepareForExecutionFunctor
{
  template <typename DeviceAdapter>
  VTKM_CONT bool operator()(DeviceAdapter, const CellLocatorRectilinearGrid& contLocator) const
  {
    auto* execObject = new vtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter>(
      contLocator.PlaneSize,
      contLocator.RowSize,
      contLocator.GetCellSet().template Cast<StructuredType>(),
      contLocator.GetCoordinates().GetData().template Cast<RectilinearType>(),
      DeviceAdapter());
    contLocator.ExecutionObjectHandle.Reset(execObject);

    return true;
  }
};

const vtkm::exec::CellLocator* CellLocatorRectilinearGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  if (!vtkm::cont::TryExecuteOnDevice(device, PrepareForExecutionFunctor(), *this))
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorRectilinearGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}
}
} // vtkm::cont
