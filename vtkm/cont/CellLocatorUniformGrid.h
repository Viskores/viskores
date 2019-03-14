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
  CellLocatorUniformGrid()
  {
#ifdef VTKM_CUDA
    CudaStackSizeBackup = 0;
    cudaDeviceGetLimit(&CudaStackSizeBackup, cudaLimitStackSize);
//std::cout<<"Initial stack size: "<<CudaStackSizeBackup<<std::endl;
//    std::cout<<"Increase stack size"<<std::endl;
//    cudaDeviceSetLimit(cudaLimitStackSize, 1024 * 64);
#endif
  }
  VTKM_CONT
  ~CellLocatorUniformGrid()
  {
#ifdef VTKM_CUDA
    if (CudaStackSizeBackup > 0)
    {
      //std::cout<<"DE-Increase stack size "<<CudaStackSizeBackup<<std::endl;
      cudaDeviceSetLimit(cudaLimitStackSize, CudaStackSizeBackup);
      CudaStackSizeBackup = 0;
    }
#endif
  }

  VTKM_CONT
  void Build() override
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
                          contLocator.RangeTransform,
                          contLocator.CellDims,
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
#ifdef VTKM_CUDA
    //std::cout<<"Increase stack size"<<std::endl;
    cudaDeviceSetLimit(cudaLimitStackSize, 1024 * 64);
#endif
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
  vtkm::Vec<vtkm::FloatDefault, 3> RangeTransform;
  vtkm::Vec<vtkm::Id, 3> CellDims;
  mutable HandleType ExecHandle;
#ifdef VTKM_CUDA
  std::size_t CudaStackSizeBackup;
#endif
};
}
}

#endif //vtkm_cont_celllocatoruniformgrid_h
