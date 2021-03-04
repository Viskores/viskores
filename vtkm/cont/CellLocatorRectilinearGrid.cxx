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

vtkm::exec::CellLocatorRectilinearGrid CellLocatorRectilinearGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token) const
{
  this->Update();

  using ExecObjType = vtkm::exec::CellLocatorRectilinearGrid;

  if (this->Is3D)
  {
    return ExecObjType(this->PlaneSize,
                       this->RowSize,
                       this->GetCellSet().template Cast<Structured3DType>(),
                       this->GetCoordinates().GetData().template AsArrayHandle<RectilinearType>(),
                       device,
                       token);
  }
  else
  {
    return ExecObjType(this->PlaneSize,
                       this->RowSize,
                       this->GetCellSet().template Cast<Structured2DType>(),
                       this->GetCoordinates().GetData().template AsArrayHandle<RectilinearType>(),
                       device,
                       token);
  }
}

} //namespace cont
} //namespace vtkm
