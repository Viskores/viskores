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
    throw vtkm::cont::ErrorBadType("Coordinates are not uniform type.");

  if (cellSet.IsSameType(Structured2DType()))
  {
    this->Is3D = false;
    Structured2DType structuredCellSet = cellSet.Cast<Structured2DType>();
    vtkm::Id2 pointDims = structuredCellSet.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    this->PointDims = vtkm::Id3(pointDims[0], pointDims[1], 1);
  }
  else if (cellSet.IsSameType(Structured3DType()))
  {
    this->Is3D = true;
    Structured3DType structuredCellSet = cellSet.Cast<Structured3DType>();
    this->PointDims = structuredCellSet.GetSchedulingRange(vtkm::TopologyElementTagPoint());
  }
  else
  {
    throw vtkm::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }

  UniformType uniformCoords = coords.GetData().Cast<UniformType>();
  this->Origin = uniformCoords.GetPortalConstControl().GetOrigin();

  vtkm::Vec3f spacing = uniformCoords.GetPortalConstControl().GetSpacing();
  vtkm::Vec3f unitLength;
  unitLength[0] = static_cast<vtkm::FloatDefault>(this->PointDims[0] - 1);
  unitLength[1] = static_cast<vtkm::FloatDefault>(this->PointDims[1] - 1);
  unitLength[2] = static_cast<vtkm::FloatDefault>(this->PointDims[2] - 1);

  this->MaxPoint = this->Origin + spacing * unitLength;
  this->InvSpacing[0] = 1.f / spacing[0];
  this->InvSpacing[1] = 1.f / spacing[1];
  this->InvSpacing[2] = 1.f / spacing[2];

  this->CellDims[0] = this->PointDims[0] - 1;
  this->CellDims[1] = this->PointDims[1] - 1;
  this->CellDims[2] = this->PointDims[2] - 1;
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
                                             this->CellDims,
                                             this->PointDims,
                                             this->Origin,
                                             this->InvSpacing,
                                             this->MaxPoint,
                                             this->GetCoordinates().GetData());
  }
  else
  {
    success = vtkm::cont::TryExecuteOnDevice(device,
                                             CellLocatorUniformGridPrepareForExecutionFunctor<2>(),
                                             this->ExecutionObjectHandle,
                                             this->CellDims,
                                             this->PointDims,
                                             this->Origin,
                                             this->InvSpacing,
                                             this->MaxPoint,
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
