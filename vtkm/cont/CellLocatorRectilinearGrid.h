//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_cont_celllocatorrectilineargrid_h
#define vtkm_cont_celllocatorrectilineargrid_h

#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadDevice.h>

#include <vtkm/exec/CellLocatorRectilinearGrid.h>

namespace vtkm
{

namespace cont
{

class CellLocatorRectilinearGrid : public vtkm::cont::CellLocator
{
public:
  using StructuredType = vtkm::cont::CellSetStructured<3>;
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

  VTKM_CONT
  CellLocatorRectilinearGrid() = default;

  VTKM_CONT
  void Build() override
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

  struct PrepareForExecutionFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              const vtkm::cont::CellLocatorRectilinearGrid& contLocator,
                              HandleType& execLocator) const
    {
      using ExecutionType = vtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter>;
      ExecutionType* execObject =
        new ExecutionType(contLocator.PlaneSize,
                          contLocator.RowSize,
                          contLocator.GetCellSet().template Cast<StructuredType>(),
                          contLocator.GetCoordinates().GetData().template Cast<RectilinearType>(),
                          DeviceAdapter());
      execLocator.Reset(execObject);
      return true;
    }
  };

  VTKM_CONT
  const HandleType PrepareForExecutionImpl(
    const vtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success = vtkm::cont::TryExecuteOnDevice(
      deviceId, PrepareForExecutionFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("CellLocatorRectilinearGrid", deviceId);
    }
    return this->ExecHandle;
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;

  mutable HandleType ExecHandle;
};

} //namespace cont

} //namespace vtkm

#endif //vtkm_cont_celllocatorrectilineargrid_h
