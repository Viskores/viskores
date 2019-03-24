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
#include <vtkm/cont/CellLocatorUniformGrid.h>

#include <vtkm/exec/CellLocatorUniformGrid.h>

vtkm::cont::CellLocatorUniformGrid::CellLocatorUniformGrid() = default;

vtkm::cont::CellLocatorUniformGrid::~CellLocatorUniformGrid() = default;

void vtkm::cont::CellLocatorUniformGrid::Build()
{
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<UniformType>())
    throw vtkm::cont::ErrorBadType("Coordinate system is not uniform type");
  if (!cellSet.IsSameType(StructuredType()))
    throw vtkm::cont::ErrorBadType("Cell set is not 3D structured type");

  Bounds = coords.GetBounds();
  CellDims = cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagCell());

  RangeTransform[0] = static_cast<vtkm::FloatDefault>(CellDims[0]) /
    static_cast<vtkm::FloatDefault>(Bounds.X.Length());
  RangeTransform[1] = static_cast<vtkm::FloatDefault>(CellDims[1]) /
    static_cast<vtkm::FloatDefault>(Bounds.Y.Length());
  RangeTransform[2] = static_cast<vtkm::FloatDefault>(CellDims[2]) /
    static_cast<vtkm::FloatDefault>(Bounds.Z.Length());
}

struct vtkm::cont::CellLocatorUniformGrid::PrepareForExecutionFunctor
{
  template <typename DeviceAdapter>
  VTKM_CONT bool operator()(DeviceAdapter,
                            const vtkm::cont::CellLocatorUniformGrid& contLocator,
                            HandleType& execLocator) const
  {
    using ExecutionType = vtkm::exec::CellLocatorUniformGrid<DeviceAdapter>;
    ExecutionType* execObject =
      new ExecutionType(contLocator.Bounds,
                        contLocator.RangeTransform,
                        contLocator.CellDims,
                        contLocator.GetCellSet().template Cast<StructuredType>(),
                        contLocator.GetCoordinates().GetData(),
                        DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};

const vtkm::cont::CellLocator::HandleType
vtkm::cont::CellLocatorUniformGrid::PrepareForExecutionImpl(
  const vtkm::cont::DeviceAdapterId deviceId) const
{
  const bool success =
    vtkm::cont::TryExecuteOnDevice(deviceId, PrepareForExecutionFunctor(), *this, this->ExecHandle);
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", deviceId);
  }
  return this->ExecHandle;
}
