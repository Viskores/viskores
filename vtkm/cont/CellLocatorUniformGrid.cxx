//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

namespace
{
struct CellLocatorUniformGridPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename... Args>
  VTKM_CONT bool operator()(DeviceAdapter,
                            vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>& execLocator,
                            Args&&... args) const
  {
    using ExecutionType = vtkm::exec::CellLocatorUniformGrid<DeviceAdapter>;
    ExecutionType* execObject = new ExecutionType(std::forward<Args>(args)..., DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};
}

const vtkm::exec::CellLocator* CellLocatorUniformGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  const bool success =
    vtkm::cont::TryExecuteOnDevice(device,
                                   CellLocatorUniformGridPrepareForExecutionFunctor(),
                                   this->ExecutionObjectHandle,
                                   this->Bounds,
                                   this->RangeTransform,
                                   this->CellDims,
                                   this->GetCellSet().template Cast<StructuredType>(),
                                   this->GetCoordinates().GetData());
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}

} //namespace cont
} //namespace vtkm
