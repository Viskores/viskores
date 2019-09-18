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
#include <vtkm/cont/PointLocatorUniformGrid.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

class BinPointsWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coord, FieldOut label);

  using ExecutionSignature = void(_1, _2);

  VTKM_CONT
  BinPointsWorklet(vtkm::Vec3f min, vtkm::Vec3f max, vtkm::Id3 dims)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
  {
  }

  template <typename CoordVecType, typename IdType>
  VTKM_EXEC void operator()(const CoordVecType& coord, IdType& label) const
  {
    vtkm::Id3 ijk = (coord - Min) / Dxdydz;
    ijk = vtkm::Max(ijk, vtkm::Id3(0));
    ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));
    label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
  }

private:
  vtkm::Vec3f Min;
  vtkm::Id3 Dims;
  vtkm::Vec3f Dxdydz;
};

} // internal

void PointLocatorUniformGrid::Build()
{
  if (this->IsRangeInvalid())
  {
    this->Range = this->GetCoordinates().GetRange();
  }

  auto rmin = vtkm::make_Vec(static_cast<vtkm::FloatDefault>(this->Range[0].Min),
                             static_cast<vtkm::FloatDefault>(this->Range[1].Min),
                             static_cast<vtkm::FloatDefault>(this->Range[2].Min));
  auto rmax = vtkm::make_Vec(static_cast<vtkm::FloatDefault>(this->Range[0].Max),
                             static_cast<vtkm::FloatDefault>(this->Range[1].Max),
                             static_cast<vtkm::FloatDefault>(this->Range[2].Max));

  // generate unique id for each input point
  vtkm::cont::ArrayHandleCounting<vtkm::Id> pointCounting(
    0, 1, this->GetCoordinates().GetNumberOfValues());
  vtkm::cont::ArrayCopy(pointCounting, this->PointIds);

  using internal::BinPointsWorklet;

  // bin points into cells and give each of them the cell id.
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  BinPointsWorklet cellIdWorklet(rmin, rmax, this->Dims);
  vtkm::worklet::DispatcherMapField<BinPointsWorklet> dispatchCellId(cellIdWorklet);
  dispatchCellId.Invoke(this->GetCoordinates(), cellIds);

  // Group points of the same cell together by sorting them according to the cell ids
  vtkm::cont::Algorithm::SortByKey(cellIds, this->PointIds);

  // for each cell, find the lower and upper bound of indices to the sorted point ids.
  vtkm::cont::ArrayHandleCounting<vtkm::Id> cell_ids_counting(
    0, 1, this->Dims[0] * this->Dims[1] * this->Dims[2]);
  vtkm::cont::Algorithm::UpperBounds(cellIds, cell_ids_counting, this->CellUpper);
  vtkm::cont::Algorithm::LowerBounds(cellIds, cell_ids_counting, this->CellLower);
}

struct PointLocatorUniformGrid::PrepareExecutionObjectFunctor
{
  template <typename DeviceAdapter>
  VTKM_CONT bool operator()(DeviceAdapter,
                            const vtkm::cont::PointLocatorUniformGrid& self,
                            ExecutionObjectHandleType& handle) const
  {
    auto rmin = vtkm::make_Vec(static_cast<vtkm::FloatDefault>(self.Range[0].Min),
                               static_cast<vtkm::FloatDefault>(self.Range[1].Min),
                               static_cast<vtkm::FloatDefault>(self.Range[2].Min));
    auto rmax = vtkm::make_Vec(static_cast<vtkm::FloatDefault>(self.Range[0].Max),
                               static_cast<vtkm::FloatDefault>(self.Range[1].Max),
                               static_cast<vtkm::FloatDefault>(self.Range[2].Max));
    vtkm::exec::PointLocatorUniformGrid<DeviceAdapter>* h =
      new vtkm::exec::PointLocatorUniformGrid<DeviceAdapter>(
        rmin,
        rmax,
        self.Dims,
        self.GetCoordinates().GetData().PrepareForInput(DeviceAdapter()),
        self.PointIds.PrepareForInput(DeviceAdapter()),
        self.CellLower.PrepareForInput(DeviceAdapter()),
        self.CellUpper.PrepareForInput(DeviceAdapter()));
    handle.Reset(h);
    return true;
  }
};

VTKM_CONT void PointLocatorUniformGrid::PrepareExecutionObject(
  ExecutionObjectHandleType& execObjHandle,
  vtkm::cont::DeviceAdapterId deviceId) const
{
  const bool success =
    vtkm::cont::TryExecuteOnDevice(deviceId, PrepareExecutionObjectFunctor(), *this, execObjHandle);
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("PointLocatorUniformGrid", deviceId);
  }
}
}
} // vtkm::cont
