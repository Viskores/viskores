//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingCellLocatorRectilinearGrid_h
#define vtk_m_cont_testing_TestingCellLocatorRectilinearGrid_h

#include <random>
#include <string>

#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/CellLocatorRectilinearGrid.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

template <typename DeviceAdapter>
class LocatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using AxisPortalType = typename AxisHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using RectilinearPortalType =
    typename RectilinearType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  LocatorWorklet(vtkm::Bounds& bounds, vtkm::Id3& dims, const RectilinearType& coords)
    : Bounds(bounds)
    , Dims(dims)
  {
    RectilinearPortalType coordsPortal = coords.PrepareForInput(DeviceAdapter());
    xAxis = coordsPortal.GetFirstPortal();
    yAxis = coordsPortal.GetSecondPortal();
    zAxis = coordsPortal.GetThirdPortal();
  }

  using ControlSignature =
    void(FieldIn pointIn, ExecObject locator, FieldOut cellId, FieldOut parametric, FieldOut match);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename PointType>
  VTKM_EXEC vtkm::Id CalculateCellId(const PointType& point) const
  {
    if (!Bounds.Contains(point))
      return -1;
    vtkm::Id3 logical(-1, -1, -1);
    // Linear search in the coordinates.
    vtkm::Id index;
    /*Get floor X location*/
    if (point[0] == xAxis.Get(this->Dims[0] - 1))
      logical[0] = this->Dims[0] - 1;
    else
      for (index = 0; index < this->Dims[0] - 1; index++)
        if (xAxis.Get(index) <= point[0] && point[0] < xAxis.Get(index + 1))
        {
          logical[0] = index;
          break;
        }
    /*Get floor Y location*/
    if (point[1] == yAxis.Get(this->Dims[1] - 1))
      logical[1] = this->Dims[1] - 1;
    else
      for (index = 0; index < this->Dims[1] - 1; index++)
        if (yAxis.Get(index) <= point[1] && point[1] < yAxis.Get(index + 1))
        {
          logical[1] = index;
          break;
        }
    /*Get floor Z location*/
    if (point[2] == zAxis.Get(this->Dims[2] - 1))
      logical[2] = this->Dims[2] - 1;
    else
      for (index = 0; index < this->Dims[2] - 1; index++)
        if (zAxis.Get(index) <= point[2] && point[2] < zAxis.Get(index + 1))
        {
          logical[2] = index;
          break;
        }
    if (logical[0] == -1 || logical[1] == -1 || logical[2] == -1)
      return -1;
    return logical[2] * (Dims[0] - 1) * (Dims[1] - 1) + logical[1] * (Dims[0] - 1) + logical[0];
  }

  template <typename PointType, typename LocatorType>
  VTKM_EXEC void operator()(const PointType& pointIn,
                            const LocatorType& locator,
                            vtkm::Id& cellId,
                            PointType& parametric,
                            bool& match) const
  {
    vtkm::Id calculated = CalculateCellId(pointIn);
    locator->FindCell(pointIn, cellId, parametric, (*this));
    match = (calculated == cellId);
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Id3 Dims;
  AxisPortalType xAxis;
  AxisPortalType yAxis;
  AxisPortalType zAxis;
};

template <typename DeviceAdapter>
class TestingCellLocatorRectilinearGrid
{
public:
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  void TestTest() const
  {
    vtkm::cont::DataSetBuilderRectilinear dsb;
    std::vector<vtkm::Float32> X(4), Y(3), Z(5);
    X[0] = 0.0f;
    X[1] = 1.0f;
    X[2] = 3.0f;
    X[3] = 4.0f;
    Y[0] = 0.0f;
    Y[1] = 1.0f;
    Y[2] = 2.0f;
    Z[0] = 0.0f;
    Z[1] = 1.0f;
    Z[2] = 3.0f;
    Z[3] = 5.0f;
    Z[4] = 6.0f;
    vtkm::cont::DataSet dataset = dsb.Create(X, Y, Z);

    using StructuredType = vtkm::cont::CellSetStructured<3>;
    using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
    using RectilinearType =
      vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

    vtkm::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
    vtkm::cont::DynamicCellSet cellSet = dataset.GetCellSet();
    vtkm::Bounds bounds = coords.GetBounds();
    vtkm::Id3 dims =
      cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());

    // Generate some sample points.
    using PointType = vtkm::Vec3f;
    std::vector<PointType> pointsVec;
    std::default_random_engine dre;
    std::uniform_real_distribution<vtkm::Float32> xCoords(0.0f, 4.0f);
    std::uniform_real_distribution<vtkm::Float32> yCoords(0.0f, 2.0f);
    std::uniform_real_distribution<vtkm::Float32> zCoords(0.0f, 6.0f);
    for (size_t i = 0; i < 10; i++)
    {
      PointType point = vtkm::make_Vec(xCoords(dre), yCoords(dre), zCoords(dre));
      pointsVec.push_back(point);
    }

    vtkm::cont::ArrayHandle<PointType> points = vtkm::cont::make_ArrayHandle(pointsVec);

    // Initialize locator
    vtkm::cont::CellLocatorRectilinearGrid locator;
    locator.SetCoordinates(coords);
    locator.SetCellSet(cellSet);
    locator.Update();

    // Query the points using the locator.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    vtkm::cont::ArrayHandle<PointType> parametric;
    vtkm::cont::ArrayHandle<bool> match;
    LocatorWorklet<DeviceAdapter> worklet(
      bounds, dims, coords.GetData().template Cast<RectilinearType>());

    vtkm::worklet::DispatcherMapField<LocatorWorklet<DeviceAdapter>> dispatcher(worklet);
    dispatcher.SetDevice(DeviceAdapter());
    dispatcher.Invoke(points, locator, cellIds, parametric, match);

    auto matchPortal = match.GetPortalConstControl();
    for (vtkm::Id index = 0; index < match.GetNumberOfValues(); index++)
    {
      VTKM_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
    }
  }

  void operator()() const
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    this->TestTest();
  }
};

#endif
