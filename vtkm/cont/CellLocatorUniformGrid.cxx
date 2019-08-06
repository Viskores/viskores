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
using Structured2DType = vtkm::cont::CellSetStructured<2>;
using Structured3DType = vtkm::cont::CellSetStructured<3>;

void CellLocatorUniformGrid::Build()
{
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<UniformType>())
    throw vtkm::cont::ErrorInternal("Coordinates are not uniform.");

  if (cellSet.IsSameType(Structured2DType()))
  {
    this->Is3D = false;
    this->Bounds = coords.GetBounds();
    vtkm::Id2 cellDims =
      cellSet.Cast<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    this->CellDims = vtkm::Id3(cellDims[0], cellDims[1], 0);
    this->RangeTransform[0] = static_cast<vtkm::FloatDefault>(this->CellDims[0]) /
      static_cast<vtkm::FloatDefault>(this->Bounds.X.Length());
    this->RangeTransform[1] = static_cast<vtkm::FloatDefault>(this->CellDims[1]) /
      static_cast<vtkm::FloatDefault>(this->Bounds.Y.Length());
  }
  else if (cellSet.IsSameType(Structured3DType()))
  {
    this->Is3D = true;
    this->Bounds = coords.GetBounds();
    this->CellDims =
      cellSet.Cast<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    this->RangeTransform[0] = static_cast<vtkm::FloatDefault>(this->CellDims[0]) /
      static_cast<vtkm::FloatDefault>(this->Bounds.X.Length());
    this->RangeTransform[1] = static_cast<vtkm::FloatDefault>(this->CellDims[1]) /
      static_cast<vtkm::FloatDefault>(this->Bounds.Y.Length());
    this->RangeTransform[2] = static_cast<vtkm::FloatDefault>(this->CellDims[2]) /
      static_cast<vtkm::FloatDefault>(this->Bounds.Z.Length());
  }
  else
  {
    throw vtkm::cont::ErrorInternal("Cells are not structured.");
  }
}

namespace
{
template <vtkm::IdComponent dimensions>
struct CellLocatorUniformGridPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename... Args>
  VTKM_CONT bool operator()(DeviceAdapter,
                            vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>& execLocator,
                            Args&&... args) const
  {
    using ExecutionType = vtkm::exec::CellLocatorUniformGrid<DeviceAdapter, dimensions>;
    ExecutionType* execObject = new ExecutionType(std::forward<Args>(args)..., DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};
}

const vtkm::exec::CellLocator* CellLocatorUniformGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  bool success = true;
  if (this->Is3D)
  {
    success = vtkm::cont::TryExecuteOnDevice(device,
                                             CellLocatorUniformGridPrepareForExecutionFunctor<3>(),
                                             this->ExecutionObjectHandle,
                                             this->Bounds,
                                             this->RangeTransform,
                                             this->CellDims,
                                             this->GetCellSet().template Cast<Structured3DType>(),
                                             this->GetCoordinates().GetData());
  }
  else
  {
    success = vtkm::cont::TryExecuteOnDevice(device,
                                             CellLocatorUniformGridPrepareForExecutionFunctor<2>(),
                                             this->ExecutionObjectHandle,
                                             this->Bounds,
                                             this->RangeTransform,
                                             this->CellDims,
                                             this->GetCellSet().template Cast<Structured2DType>(),
                                             this->GetCoordinates().GetData());
  }
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}

} //namespace cont
} //namespace vtkm
