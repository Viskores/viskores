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

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/exec/CellLocatorUniformGrid.h>

namespace vtkm
{
namespace cont
{
CellLocatorUniformGrid::CellLocatorUniformGrid() = default;

CellLocatorUniformGrid::~CellLocatorUniformGrid() = default;

using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
using StructuredType = vtkm::cont::CellSetStructured<3>;

void CellLocatorUniformGrid::Build()
{
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<UniformType>())
    throw vtkm::cont::ErrorBadType("Coordinate system is not uniform type");
  if (!cellSet.IsSameType(StructuredType()))
    throw vtkm::cont::ErrorBadType("Cell set is not 3D structured type");

  this->Bounds = coords.GetBounds();
  this->CellDims =
    cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagCell());

  this->RangeTransform[0] = static_cast<vtkm::FloatDefault>(this->CellDims[0]) /
    static_cast<vtkm::FloatDefault>(this->Bounds.X.Length());
  this->RangeTransform[1] = static_cast<vtkm::FloatDefault>(this->CellDims[1]) /
    static_cast<vtkm::FloatDefault>(this->Bounds.Y.Length());
  this->RangeTransform[2] = static_cast<vtkm::FloatDefault>(this->CellDims[2]) /
    static_cast<vtkm::FloatDefault>(this->Bounds.Z.Length());
}

struct CellLocatorUniformGrid::PrepareForExecutionFunctor
{
  template <typename DeviceAdapter>
  VTKM_CONT bool operator()(DeviceAdapter, const CellLocatorUniformGrid& contLocator) const
  {
    auto* execObject = new vtkm::exec::CellLocatorUniformGrid<DeviceAdapter>(
      contLocator.Bounds,
      contLocator.RangeTransform,
      contLocator.CellDims,
      contLocator.GetCellSet().template Cast<StructuredType>(),
      contLocator.GetCoordinates().GetData(),
      DeviceAdapter());
    contLocator.ExecutionObjectHandle.Reset(execObject);

    return true;
  }
};

const vtkm::exec::CellLocator* CellLocatorUniformGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  if (!vtkm::cont::TryExecuteOnDevice(device, PrepareForExecutionFunctor(), *this))
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}
}
} // vtkm::cont
