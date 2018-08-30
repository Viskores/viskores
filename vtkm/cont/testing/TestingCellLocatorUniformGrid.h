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
#ifndef vtk_m_cont_testing_TestingCellLocatorUniformGrid_h
#define vtk_m_cont_testing_TestingCellLocatorUniformGrid_h

#include <random>
#include <string>

#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/CellLocatorUniformGrid.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

class LocatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  LocatorWorklet(vtkm::Bounds& bounds_, vtkm::Vec<vtkm::Id, 3>& dims_)
    : bounds(bounds_)
    , dims(dims_)
  {
  }

  using ControlSignature = void(FieldIn<> pointIn,
                                ExecObject locator,
                                FieldOut<> cellId,
                                FieldOut<> parametric,
                                FieldOut<> match);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename PointType>
  VTKM_EXEC vtkm::Id CalculateCellId(const PointType& point) const
  {
    if (!bounds.Contains(point))
      return -1;
    vtkm::Vec<vtkm::Id, 3> logical;
    logical[0] = static_cast<vtkm::Id>(
      vtkm::Floor((point[0] / bounds.X.Length()) * static_cast<vtkm::FloatDefault>(dims[0] - 1)));
    logical[1] = static_cast<vtkm::Id>(
      vtkm::Floor((point[1] / bounds.Y.Length()) * static_cast<vtkm::FloatDefault>(dims[1] - 1)));
    logical[2] = static_cast<vtkm::Id>(
      vtkm::Floor((point[2] / bounds.Z.Length()) * static_cast<vtkm::FloatDefault>(dims[2] - 1)));
    return logical[2] * (dims[0] - 1) * (dims[1] - 1) + logical[1] * (dims[0] - 1) + logical[0];
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
  vtkm::Bounds bounds;
  vtkm::Vec<vtkm::Id, 3> dims;
};

template <typename DeviceAdapter>
class TestingCellLocatorUniformGrid
{
public:
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  void TestTest() const
  {
    vtkm::cont::DataSet dataset = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();
    vtkm::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
    vtkm::cont::DynamicCellSet cellSet = dataset.GetCellSet();

    vtkm::Bounds bounds = coords.GetBounds();
    std::cout << "X bounds : " << bounds.X.Min << " to " << bounds.X.Max << std::endl;
    std::cout << "Y bounds : " << bounds.Y.Min << " to " << bounds.Y.Max << std::endl;
    std::cout << "Z bounds : " << bounds.Z.Min << " to " << bounds.Z.Max << std::endl;

    using StructuredType = vtkm::cont::CellSetStructured<3>;
    vtkm::Vec<vtkm::Id, 3> dims =
      cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
    std::cout << "Dimensions of dataset : " << dims << std::endl;

    vtkm::cont::CellLocatorUniformGrid locator;
    locator.SetCoordinates(coords);
    locator.SetCellSet(cellSet);

    locator.Update();

    // Generate some sample points.
    using PointType = vtkm::Vec<vtkm::FloatDefault, 3>;
    std::vector<PointType> pointsVec;
    std::default_random_engine dre;
    std::uniform_real_distribution<vtkm::Float32> inBounds(0.0f, 4.0f);
    for (size_t i = 0; i < 10; i++)
    {
      PointType point = vtkm::make_Vec(inBounds(dre), inBounds(dre), inBounds(dre));
      pointsVec.push_back(point);
    }
    std::uniform_real_distribution<vtkm::Float32> outBounds(4.0f, 5.0f);
    for (size_t i = 0; i < 5; i++)
    {
      PointType point = vtkm::make_Vec(outBounds(dre), outBounds(dre), outBounds(dre));
      pointsVec.push_back(point);
    }

    vtkm::cont::ArrayHandle<PointType> points = vtkm::cont::make_ArrayHandle(pointsVec);
    // Query the points using the locators.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    vtkm::cont::ArrayHandle<PointType> parametric;
    vtkm::cont::ArrayHandle<bool> match;
    LocatorWorklet worklet(bounds, dims);
    vtkm::worklet::DispatcherMapField<LocatorWorklet> dispatcher(worklet);
    dispatcher.SetDevice(DeviceAdapter());
    dispatcher.Invoke(points, locator, cellIds, parametric, match);

    auto matchPortal = match.GetPortalConstControl();
    for (vtkm::Id index = 0; index < match.GetNumberOfValues(); index++)
    {
      VTKM_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
    }
    std::cout << "Test finished successfully." << std::endl;
  }

  void operator()() const
  {
    vtkm::cont::GetGlobalRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    this->TestTest();
  }
};

#endif
