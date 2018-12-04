//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_PointLocatorUniformGrid_h
#define vtk_m_cont_PointLocatorUniformGrid_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/PointLocator.h>
#include <vtkm/exec/PointLocatorUniformGrid.h>

namespace vtkm
{
namespace cont
{
class PointLocatorUniformGrid : public vtkm::cont::PointLocator
{
public:
  PointLocatorUniformGrid(const vtkm::Vec<vtkm::FloatDefault, 3>& _min,
                          const vtkm::Vec<vtkm::FloatDefault, 3>& _max,
                          const vtkm::Vec<vtkm::Id, 3>& _dims)
    : PointLocator()
    , Min(_min)
    , Max(_max)
    , Dims(_dims)
  {
  }

  class BinPointsWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<> coord, FieldOut<> label);

    using ExecutionSignature = void(_1, _2);

    VTKM_CONT
    BinPointsWorklet(vtkm::Vec<vtkm::FloatDefault, 3> _min,
                     vtkm::Vec<vtkm::FloatDefault, 3> _max,
                     vtkm::Vec<vtkm::Id, 3> _dims)
      : Min(_min)
      , Dims(_dims)
      , Dxdydz((_max - Min) / Dims)
    {
    }

    template <typename CoordVecType, typename IdType>
    VTKM_EXEC void operator()(const CoordVecType& coord, IdType& label) const
    {
      vtkm::Vec<vtkm::Id, 3> ijk = (coord - Min) / Dxdydz;
      label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
    }

  private:
    vtkm::Vec<vtkm::FloatDefault, 3> Min;
    vtkm::Vec<vtkm::Id, 3> Dims;
    vtkm::Vec<vtkm::FloatDefault, 3> Dxdydz;
  };

  void Build() override
  {
    // Save training data points.
    vtkm::cont::ArrayCopy(this->GetCoords().GetData(), this->coords);

    // generate unique id for each input point
    vtkm::cont::ArrayHandleCounting<vtkm::Id> pointCounting(0, 1, this->coords.GetNumberOfValues());
    vtkm::cont::ArrayCopy(pointCounting, this->pointIds);

    // bin points into cells and give each of them the cell id.
    BinPointsWorklet cellIdWorklet(this->Min, this->Max, this->Dims);
    vtkm::worklet::DispatcherMapField<BinPointsWorklet> dispatchCellId(cellIdWorklet);
    dispatchCellId.Invoke(this->coords, this->cellIds);

    // Group points of the same cell together by sorting them according to the cell ids
    vtkm::cont::Algorithm::SortByKey(this->cellIds, this->pointIds);

    // for each cell, find the lower and upper bound of indices to the sorted point ids.
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cell_ids_counting(
      0, 1, this->Dims[0] * this->Dims[1] * this->Dims[2]);
    vtkm::cont::Algorithm::UpperBounds(this->cellIds, cell_ids_counting, this->cellUpper);
    vtkm::cont::Algorithm::LowerBounds(this->cellIds, cell_ids_counting, this->cellLower);
  }


  using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::PointLocator>;

  struct PrepareForExecutionFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              const vtkm::cont::PointLocatorUniformGrid& self,
                              HandleType& handle) const
    {
      vtkm::exec::PointLocatorUniformGrid<DeviceAdapter>* h =
        new vtkm::exec::PointLocatorUniformGrid<DeviceAdapter>(
          self.Min,
          self.Max,
          self.Dims,
          self.coords.PrepareForInput(DeviceAdapter()),
          self.pointIds.PrepareForInput(DeviceAdapter()),
          self.cellLower.PrepareForInput(DeviceAdapter()),
          self.cellUpper.PrepareForInput(DeviceAdapter()));
      handle.Reset(h);
      return true;
    }
  };

  VTKM_CONT
  const HandleType PrepareForExecutionImp(vtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success =
      vtkm::cont::TryExecuteOnDevice(deviceId, PrepareForExecutionFunctor(), *this, ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("PointLocatorUniformGrid", deviceId);
    }
    return ExecHandle;
  }

private:
  vtkm::Vec<vtkm::FloatDefault, 3> Min;
  vtkm::Vec<vtkm::FloatDefault, 3> Max;
  vtkm::Vec<vtkm::Id, 3> Dims;

  // TODO: how to convert CoordinateSystem to ArrayHandle<Vec<Float, 3>>?
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> coords;
  vtkm::cont::ArrayHandle<vtkm::Id> pointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<vtkm::Id> cellLower;
  vtkm::cont::ArrayHandle<vtkm::Id> cellUpper;

  // TODO: std::unique_ptr/std::shared_ptr?
  mutable HandleType ExecHandle;
};
}
}
#endif //vtk_m_cont_PointLocatorUniformGrid_h
