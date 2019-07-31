//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  LocatorWorklet(vtkm::Bounds& bounds, vtkm::Id3& cellDims)
    : Bounds(bounds)
    , CellDims(cellDims)
  {
  }

  using ControlSignature =
    void(FieldIn pointIn, ExecObject locator, FieldOut cellId, FieldOut parametric, FieldOut match);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename PointType>
  VTKM_EXEC vtkm::Id CalculateCellId(const PointType& point) const
  {
    if (!Bounds.Contains(point))
      return -1;

    vtkm::Id3 logical;
    logical[0] = (point[0] == Bounds.X.Max)
      ? CellDims[0] - 1
      : static_cast<vtkm::Id>(vtkm::Floor((point[0] / Bounds.X.Length()) *
                                          static_cast<vtkm::FloatDefault>(CellDims[0])));
    logical[1] = (point[1] == Bounds.Y.Max)
      ? CellDims[1] - 1
      : static_cast<vtkm::Id>(vtkm::Floor((point[1] / Bounds.Y.Length()) *
                                          static_cast<vtkm::FloatDefault>(CellDims[1])));
    logical[2] = (point[2] == Bounds.Z.Max)
      ? CellDims[2] - 1
      : static_cast<vtkm::Id>(vtkm::Floor((point[2] / Bounds.Z.Length()) *
                                          static_cast<vtkm::FloatDefault>(CellDims[2])));

    return logical[2] * CellDims[0] * CellDims[1] + logical[1] * CellDims[0] + logical[0];
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
  vtkm::Id3 CellDims;
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
    vtkm::Id3 cellDims =
      cellSet.Cast<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    std::cout << "Dimensions of dataset : " << cellDims << std::endl;

    vtkm::cont::CellLocatorUniformGrid locator;
    locator.SetCoordinates(coords);
    locator.SetCellSet(cellSet);

    locator.Update();

    // Generate some sample points.
    using PointType = vtkm::Vec3f;
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
    std::uniform_real_distribution<vtkm::Float32> outBounds2(-1.0f, 0.0f);
    for (size_t i = 0; i < 5; i++)
    {
      PointType point = vtkm::make_Vec(outBounds2(dre), outBounds2(dre), outBounds2(dre));
      pointsVec.push_back(point);
    }

    // Add points right on the boundary.
    pointsVec.push_back(vtkm::make_Vec(0, 0, 0));
    pointsVec.push_back(vtkm::make_Vec(4, 4, 4));
    pointsVec.push_back(vtkm::make_Vec(4, 0, 0));
    pointsVec.push_back(vtkm::make_Vec(0, 4, 0));
    pointsVec.push_back(vtkm::make_Vec(0, 0, 4));
    pointsVec.push_back(vtkm::make_Vec(4, 4, 0));
    pointsVec.push_back(vtkm::make_Vec(0, 4, 4));
    pointsVec.push_back(vtkm::make_Vec(4, 0, 4));

    vtkm::cont::ArrayHandle<PointType> points = vtkm::cont::make_ArrayHandle(pointsVec);
    // Query the points using the locators.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    vtkm::cont::ArrayHandle<PointType> parametric;
    vtkm::cont::ArrayHandle<bool> match;
    LocatorWorklet worklet(bounds, cellDims);
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
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    this->TestTest();
  }
};

#endif
