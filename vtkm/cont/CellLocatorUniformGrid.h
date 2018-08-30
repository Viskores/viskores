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
#ifndef vtkm_cont_celllocatoruniformgrid_h
#define vtkm_cont_celllocatoruniformgrid_h

#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadDevice.h>

#include <vtkm/exec/CellLocatorUniformGrid.h>

namespace vtkm
{

namespace cont
{

class CellLocatorUniformGrid : public vtkm::cont::CellLocator
{
public:
  VTKM_CONT
  void Build() override
  {
    vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
    vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

    if (!coords.GetData().IsType<UniformType>())
      throw vtkm::cont::ErrorInternal("CellSet is not 3D Structured Type");
    if (!cellSet.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("CellSet is not 3D Structured Type");

    Bounds = coords.GetBounds();
    Dims = cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());

    RangeTransform[0] = static_cast<vtkm::FloatDefault>((Dims[0] - 1.0l) / (Bounds.X.Lenght()));
    RangeTransform[1] = static_cast<vtkm::FloatDefault>((Dims[1] - 1.0l) / (Bounds.Y.Length()));
    RangeTransform[2] = static_cast<vtkm::FloatDefault>((Dims[2] - 1.0l) / (Bounds.Z.Length()));

    // Since we are calculating the cell Id, and the number of cells is
    // 1 less than the number of points in each direction, the -1 from Dims
    // is necessary.
    PlaneSize = (Dims[0] - 1) * (Dims[1] - 1);
    RowSize = (Dims[0] - 1);
  }

  struct PrepareForExecutionFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              const vtkm::cont::CellLocatorUniformGrid& contLocator,
                              HandleType& execLocator) const
    {
      using ExecutionType = vtkm::exec::CellLocatorUniformGrid<DeviceAdapter>;
      ExecutionType* execObject =
        new ExecutionType(contLocator.Bounds,
                          contLocator.Dims,
                          contLocator.RangeTransform,
                          contLocator.PlaneSize,
                          contLocator.RowSize,
                          contLocator.GetCellSet().template Cast<StructuredType>(),
                          contLocator.GetCoordinates().GetData(),
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
      throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", deviceId);
    }
    return this->ExecHandle;
  }

private:
  using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using StructuredType = vtkm::cont::CellSetStructured<3>;

  vtkm::Bounds Bounds;
  vtkm::Vec<vtkm::Id, 3> Dims;
  vtkm::Vec<vtkm::FloatDefault, 3> RangeTransform;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  mutable HandleType ExecHandle;
};
}
}

#endif //vtkm_cont_celllocatoruniformgrid_h
