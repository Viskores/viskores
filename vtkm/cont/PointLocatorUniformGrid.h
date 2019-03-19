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

namespace internal
{
namespace point_locator_uniform_grid
{

class BinPointsWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coord, FieldOut label);

  using ExecutionSignature = void(_1, _2);

  VTKM_CONT
  BinPointsWorklet(vtkm::Vec<vtkm::FloatDefault, 3> min,
                   vtkm::Vec<vtkm::FloatDefault, 3> max,
                   vtkm::Vec<vtkm::Id, 3> dims)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
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
}
} // internal::point_locator_uniform_grid

class PointLocatorUniformGrid : public vtkm::cont::PointLocator
{
public:
  using RangeType = vtkm::Vec<vtkm::Range, 3>;

  PointLocatorUniformGrid() = default;

  void SetRange(const RangeType& range)
  {
    if (this->Range != range)
    {
      this->Range = range;
      this->SetModified();
    }
  }

  const RangeType& GetRange() const { return this->Range; }

  void SetComputeRangeFromCoordinates()
  {
    if (!this->IsRangeInvalid())
    {
      this->Range = { { 0.0, -1.0 } };
      this->SetModified();
    }
  }

  void SetNumberOfBins(const vtkm::Id3& bins)
  {
    if (this->Dims != bins)
    {
      this->Dims = bins;
      this->SetModified();
    }
  }

  const vtkm::Id3& GetNumberOfBins() const { return this->Dims; }

protected:
  void Build() override
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

    using internal::point_locator_uniform_grid::BinPointsWorklet;

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

  struct PrepareExecutionObjectFunctor
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

  VTKM_CONT void PrepareExecutionObject(ExecutionObjectHandleType& execObjHandle,
                                        vtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success = vtkm::cont::TryExecuteOnDevice(
      deviceId, PrepareExecutionObjectFunctor(), *this, execObjHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("PointLocatorUniformGrid", deviceId);
    }
  }

  bool IsRangeInvalid() const
  {
    return (this->Range[0].Max < this->Range[0].Min) || (this->Range[1].Max < this->Range[1].Min) ||
      (this->Range[2].Max < this->Range[2].Min);
  }

private:
  RangeType Range = { { 0.0, -1.0 } };
  vtkm::Id3 Dims = { 32 };

  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> CellLower;
  vtkm::cont::ArrayHandle<vtkm::Id> CellUpper;
};
}
}
#endif //vtk_m_cont_PointLocatorUniformGrid_h
