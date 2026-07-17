//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <string>

#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Invoker.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/exec/CellLocatorRectilinearGrid.h>

#include <viskores/worklet/WorkletMapField.h>

namespace
{

using AxisHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
using RectilinearType =
  viskores::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
using RectilinearPortalType = typename RectilinearType::ReadPortalType;

class LocatorWorklet : public viskores::worklet::WorkletMapField
{
public:
  LocatorWorklet(viskores::Bounds& bounds, viskores::Id3& dims)
    : Bounds(bounds)
    , Dims(dims)
  {
  }

  using ControlSignature = void(FieldIn pointIn,
                                ExecObject locator,
                                WholeArrayIn rectilinearCoords,
                                FieldOut cellId,
                                FieldOut parametric,
                                FieldOut match);

  template <typename PointType>
  VISKORES_EXEC viskores::Id CalculateCellId(const PointType& point,
                                             const RectilinearPortalType& coordsPortal) const
  {
    auto xAxis = coordsPortal.GetFirstPortal();
    auto yAxis = coordsPortal.GetSecondPortal();
    auto zAxis = coordsPortal.GetThirdPortal();

    if (!Bounds.Contains(point))
      return -1;
    viskores::Id3 logical(-1, -1, -1);
    // Linear search in the coordinates.
    viskores::Id index;
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

  template <typename PointType, typename LocatorType, typename CoordPortalType>
  VISKORES_EXEC void operator()(const PointType& pointIn,
                                const LocatorType& locator,
                                const CoordPortalType& coordsPortal,
                                viskores::Id& cellId,
                                PointType& parametric,
                                bool& match) const
  {
    // Note that CoordPortalType is actually a RectilinearPortalType wrapped in an
    // ExecutionWholeArrayConst. We need to get out the actual portal.
    viskores::Id calculated = CalculateCellId(pointIn, coordsPortal);
    viskores::ErrorCode status = locator.FindCell(pointIn, cellId, parametric);
    viskores::Id cellIdOnly = -1;
    viskores::ErrorCode statusId = locator.FindCellId(pointIn, cellIdOnly);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      match = false;
      return;
    }
    if (statusId != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(statusId));
      match = false;
      return;
    }
    match = (calculated == cellId) && (cellId == cellIdOnly);
  }

private:
  viskores::Bounds Bounds;
  viskores::Id3 Dims;
};

class FindCellWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointIn,
                                ExecObject locator,
                                FieldOut cellId,
                                FieldOut parametric);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename PointType, typename LocatorType>
  VISKORES_EXEC void operator()(const PointType& pointIn,
                                const LocatorType& locator,
                                viskores::Id& cellId,
                                PointType& parametric) const
  {
    if (locator.FindCell(pointIn, cellId, parametric) != viskores::ErrorCode::Success)
      cellId = -1;
  }
};

void TestTest()
{
  viskores::cont::Invoker invoke;

  viskores::cont::DataSetBuilderRectilinear dsb;
  std::vector<viskores::Float32> X(4), Y(3), Z(5);
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
  viskores::cont::DataSet dataset = dsb.Create(X, Y, Z);

  using StructuredType = viskores::cont::CellSetStructured<3>;

  viskores::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
  viskores::cont::UnknownCellSet cellSet = dataset.GetCellSet();
  viskores::Bounds bounds = coords.GetBounds();
  viskores::Id3 dims =
    cellSet.AsCellSet<StructuredType>().GetSchedulingRange(viskores::TopologyElementTagPoint());

  // Generate some sample points.
  using PointType = viskores::Vec3f;
  std::vector<PointType> pointsVec;
  std::default_random_engine dre;
  std::uniform_real_distribution<viskores::Float32> xCoords(0.0f, 4.0f);
  std::uniform_real_distribution<viskores::Float32> yCoords(0.0f, 2.0f);
  std::uniform_real_distribution<viskores::Float32> zCoords(0.0f, 6.0f);
  for (size_t i = 0; i < 10; i++)
  {
    PointType point = viskores::make_Vec(xCoords(dre), yCoords(dre), zCoords(dre));
    pointsVec.push_back(point);
  }

  viskores::cont::ArrayHandle<PointType> points =
    viskores::cont::make_ArrayHandle(pointsVec, viskores::CopyFlag::Off);

  // Initialize locator
  viskores::cont::CellLocatorRectilinearGrid locator;
  locator.SetCoordinates(coords);
  locator.SetCellSet(cellSet);
  locator.Update();

  // Query the points using the locator.
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> parametric;
  viskores::cont::ArrayHandle<bool> match;
  LocatorWorklet worklet(bounds, dims);

  invoke(worklet,
         points,
         locator,
         coords.GetData().template AsArrayHandle<RectilinearType>(),
         cellIds,
         parametric,
         match);

  auto matchPortal = match.ReadPortal();
  for (viskores::Id index = 0; index < match.GetNumberOfValues(); index++)
  {
    VISKORES_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
  }
}

void TestMixedAxisDirections()
{
  std::vector<viskores::Float32> x = { 0.0f, 1.0f, 3.0f, 4.0f };
  std::vector<viskores::Float32> y = { 4.0f, 2.0f, 0.0f };
  std::vector<viskores::Float32> z = { 6.0f, 5.0f, 3.0f, 1.0f, 0.0f };
  auto dataSet = viskores::cont::DataSetBuilderRectilinear::Create(x, y, z);

  viskores::cont::CellLocatorRectilinearGrid locator;
  locator.SetCoordinates(dataSet.GetCoordinateSystem());
  locator.SetCellSet(dataSet.GetCellSet());

  using PointType = viskores::Vec3f;
  std::vector<PointType> pointsVector{ PointType{ 2.0f, 3.0f, 4.0f },
                                       PointType{ 0.0f, 4.0f, 6.0f },
                                       PointType{ 4.0f, 0.0f, 0.0f },
                                       PointType{ 2.0f, 5.0f, 4.0f } };
  auto points = viskores::cont::make_ArrayHandle(pointsVector, viskores::CopyFlag::Off);

  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> parametric;
  viskores::cont::Invoker invoke;
  invoke(FindCellWorklet{}, points, locator, cellIds, parametric);

  auto cellIdsPortal = cellIds.ReadPortal();
  auto parametricPortal = parametric.ReadPortal();
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(0) == 7);
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(0), PointType(0.5f)));
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(1) == 0);
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(1), PointType(0.0f)));
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(2) == 23);
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(2), PointType(1.0f)));
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(3) == -1);
}

void TestMixedAxisDirections2D()
{
  std::vector<viskores::Float32> x = { 0.0f, 1.0f, 3.0f };
  std::vector<viskores::Float32> y = { 4.0f, 2.0f, 0.0f };
  auto dataSet = viskores::cont::DataSetBuilderRectilinear::Create(x, y);

  viskores::cont::CellLocatorRectilinearGrid locator;
  locator.SetCoordinates(dataSet.GetCoordinateSystem());
  locator.SetCellSet(dataSet.GetCellSet());

  using PointType = viskores::Vec3f;
  std::vector<PointType> pointsVector{ PointType{ 2.0f, 3.0f, 0.0f },
                                       PointType{ 3.0f, 0.0f, 0.0f },
                                       PointType{ 2.0f, 5.0f, 0.0f } };
  auto points = viskores::cont::make_ArrayHandle(pointsVector, viskores::CopyFlag::Off);

  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> parametric;
  viskores::cont::Invoker invoke;
  invoke(FindCellWorklet{}, points, locator, cellIds, parametric);

  auto cellIdsPortal = cellIds.ReadPortal();
  auto parametricPortal = parametric.ReadPortal();
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(0) == 1);
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(0)[0], 0.5f));
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(0)[1], 0.5f));
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(1) == 3);
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(1)[0], 1.0f));
  VISKORES_TEST_ASSERT(test_equal(parametricPortal.Get(1)[1], 1.0f));
  VISKORES_TEST_ASSERT(cellIdsPortal.Get(2) == -1);
}

void TestInvalidAxes()
{
  auto checkInvalid = [](const std::vector<viskores::Float32>& x)
  {
    std::vector<viskores::Float32> y = { 0.0f, 1.0f, 2.0f };
    std::vector<viskores::Float32> z = { 0.0f, 1.0f, 2.0f };
    auto dataSet = viskores::cont::DataSetBuilderRectilinear::Create(x, y, z);

    viskores::cont::CellLocatorRectilinearGrid locator;
    locator.SetCoordinates(dataSet.GetCoordinateSystem());
    locator.SetCellSet(dataSet.GetCellSet());

    bool threw = false;
    try
    {
      locator.Update();
    }
    catch (const viskores::cont::ErrorBadValue&)
    {
      threw = true;
    }
    VISKORES_TEST_ASSERT(threw, "Invalid rectilinear axis did not throw ErrorBadValue.");
  };

  checkInvalid({ 0.0f, 2.0f, 1.0f, 3.0f });
  checkInvalid({ 0.0f, 1.0f, 1.0f, 2.0f });
}

void TestCellLocatorRectilinearGrid()
{
  TestTest();
  TestMixedAxisDirections();
  TestMixedAxisDirections2D();
  TestInvalidAxes();
}

} // anonymous namespace

int UnitTestCellLocatorRectilinearGrid(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellLocatorRectilinearGrid, argc, argv);
}
