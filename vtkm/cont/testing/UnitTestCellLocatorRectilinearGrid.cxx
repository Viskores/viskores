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

#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/CellLocatorRectilinearGrid.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace
{

using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using RectilinearType = vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
using RectilinearPortalType = typename RectilinearType::ReadPortalType;

class LocatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  LocatorWorklet(vtkm::Bounds& bounds, vtkm::Id3& dims)
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
  VTKM_EXEC vtkm::Id CalculateCellId(const PointType& point,
                                     const RectilinearPortalType& coordsPortal) const
  {
    auto xAxis = coordsPortal.GetFirstPortal();
    auto yAxis = coordsPortal.GetSecondPortal();
    auto zAxis = coordsPortal.GetThirdPortal();

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

  template <typename PointType, typename LocatorType, typename CoordPortalType>
  VTKM_EXEC void operator()(const PointType& pointIn,
                            const LocatorType& locator,
                            const CoordPortalType& coordsPortal,
                            vtkm::Id& cellId,
                            PointType& parametric,
                            bool& match) const
  {
    // Note that CoordPortalType is actually a RectilinearPortalType wrapped in an
    // ExecutionWholeArrayConst. We need to get out the actual portal.
    vtkm::Id calculated = CalculateCellId(pointIn, coordsPortal);
    vtkm::ErrorCode status = locator.FindCell(pointIn, cellId, parametric);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
      match = false;
      return;
    }
    match = (calculated == cellId);
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Id3 Dims;
};

void TestTest()
{
  vtkm::cont::Invoker invoke;

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

  vtkm::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
  vtkm::cont::UnknownCellSet cellSet = dataset.GetCellSet();
  vtkm::Bounds bounds = coords.GetBounds();
  vtkm::Id3 dims =
    cellSet.AsCellSet<StructuredType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());

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

  vtkm::cont::ArrayHandle<PointType> points =
    vtkm::cont::make_ArrayHandle(pointsVec, vtkm::CopyFlag::Off);

  // Initialize locator
  vtkm::cont::CellLocatorRectilinearGrid locator;
  locator.SetCoordinates(coords);
  locator.SetCellSet(cellSet);
  locator.Update();

  // Query the points using the locator.
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> parametric;
  vtkm::cont::ArrayHandle<bool> match;
  LocatorWorklet worklet(bounds, dims);

  invoke(worklet,
         points,
         locator,
         coords.GetData().template AsArrayHandle<RectilinearType>(),
         cellIds,
         parametric,
         match);

  auto matchPortal = match.ReadPortal();
  for (vtkm::Id index = 0; index < match.GetNumberOfValues(); index++)
  {
    VTKM_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
  }
}

} // anonymous namespace

int UnitTestCellLocatorRectilinearGrid(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTest, argc, argv);
}
