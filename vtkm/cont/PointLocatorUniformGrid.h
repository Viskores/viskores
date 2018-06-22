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

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/TryExecute.h>
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

  /// \brief Construct a 3D uniform grid for nearest neighbor search.
  ///
  /// \param coords An ArrayHandle of x, y, z coordinates of input points.
  /// \param device Tag for selecting device adapter

  //template <typename DeviceAdapter>
  //void Build(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& coords, DeviceAdapter)
  struct BuildFunctor
  {
    BuildFunctor(vtkm::cont::PointLocatorUniformGrid _self)
      : self(_self)
    {
    }

    template <typename Device>
    bool operator()(Device)
    {
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

      // Save training data points.
      //Algorithm::Copy(coords, _Coords);

      // generate unique id for each input point
      vtkm::cont::ArrayHandleCounting<vtkm::Id> pointCounting(
        0, 1, self.coords.GetNumberOfValues());
      Algorithm::Copy(pointCounting, self.pointIds);

      // bin points into cells and give each of them the cell id.
      BinPointsWorklet cellIdWorklet(self.Min, self.Max, self.Dims);
      vtkm::worklet::DispatcherMapField<BinPointsWorklet> dispatchCellId(cellIdWorklet);
      dispatchCellId.Invoke(self.coords, self._CellIds);

      // Group points of the same cell together by sorting them according to the cell ids
      Algorithm::SortByKey(self._CellIds, self.pointIds);

      // for each cell, find the lower and upper bound of indices to the sorted point ids.
      vtkm::cont::ArrayHandleCounting<vtkm::Id> cell_ids_counting(
        0, 1, self.Dims[0] * self.Dims[1] * self.Dims[2]);
      Algorithm::UpperBounds(self._CellIds, cell_ids_counting, self._CellUpper);
      Algorithm::LowerBounds(self._CellIds, cell_ids_counting, self._CellLower);

      return true;
    }

  private:
    vtkm::cont::PointLocatorUniformGrid& self;
  };

  void Build() override
  {
    BuildFunctor functor(*this);

    bool success = vtkm::cont::TryExecute(functor);
    if (!success)
    {
      throw vtkm::cont::ErrorExecution("Could not build point locator structure");
    }
  };


  using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::PointLocator>;

  VTKM_CONT
  const vtkm::exec::PointLocator* PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId) override
  {
    // TODO: call VirtualObjectHandle::PrepareForExecution() and return vtkm::exec::PointLocator
    // TODO: how to convert deviceId back to DeviceAdapter tag?
    using DeviceAdapter = vtkm::cont::DeviceAdapterTagSerial;
    vtkm::exec::PointLocatorUniformGrid* locator =
      new vtkm::exec::PointLocatorUniformGrid(Min,
                                              Max,
                                              Dims,
                                              coords.PrepareForInput(DeviceAdapter()),
                                              pointIds.PrepareForInput(DeviceAdapter()),
                                              _CellLower.PrepareForInput(DeviceAdapter()),
                                              _CellUpper.PrepareForInput(DeviceAdapter()));
    ExecHandle = new HandleType(locator, false);
    return ExecHandle->PrepareForExecution(DeviceAdapter());
  }

private:
  vtkm::Vec<vtkm::FloatDefault, 3> Min;
  vtkm::Vec<vtkm::FloatDefault, 3> Max;
  vtkm::Vec<vtkm::Id, 3> Dims;

  // TODO: how to convert CoordinateSystem to ArrayHandle<Vec<Float, 3>>?
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> coords;
  vtkm::cont::ArrayHandle<vtkm::Id> pointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> _CellIds;
  vtkm::cont::ArrayHandle<vtkm::Id> _CellLower;
  vtkm::cont::ArrayHandle<vtkm::Id> _CellUpper;

  // TODO: std::unique_ptr/std::shared_ptr?
  HandleType* ExecHandle;
};
}
}
#endif //vtk_m_cont_PointLocatorUniformGrid_h
