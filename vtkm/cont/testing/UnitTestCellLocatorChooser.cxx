//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellLocatorChooser.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

std::default_random_engine RandomGenerator;

using PointType = vtkm::Vec3f;

//-----------------------------------------------------------------------------
vtkm::cont::DataSet MakeTestDataSetUniform()
{
  return vtkm::cont::DataSetBuilderUniform::Create(
    vtkm::Id3{ 32 }, PointType{ -32.0f }, PointType{ 1.0f / 64.0f });
}

vtkm::cont::DataSet MakeTestDataSetRectilinear()
{
  std::uniform_real_distribution<vtkm::FloatDefault> coordGen(1.0f / 128.0f, 1.0f / 32.0f);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> coords[3];
  for (int i = 0; i < 3; ++i)
  {
    coords[i].Allocate(16);
    auto portal = coords[i].WritePortal();

    vtkm::FloatDefault cur = 0.0f;
    for (vtkm::Id j = 0; j < portal.GetNumberOfValues(); ++j)
    {
      cur += coordGen(RandomGenerator);
      portal.Set(j, cur);
    }
  }

  return vtkm::cont::DataSetBuilderRectilinear::Create(coords[0], coords[1], coords[2]);
}

vtkm::cont::DataSet MakeTestDataSetCurvilinear()
{
  auto recti = MakeTestDataSetRectilinear();
  auto coords = recti.GetCoordinateSystem().GetDataAsMultiplexer();

  vtkm::cont::ArrayHandle<PointType> sheared;
  sheared.Allocate(coords.GetNumberOfValues());

  auto inPortal = coords.ReadPortal();
  auto outPortal = sheared.WritePortal();
  for (vtkm::Id i = 0; i < inPortal.GetNumberOfValues(); ++i)
  {
    auto val = inPortal.Get(i);
    outPortal.Set(i, val + vtkm::make_Vec(val[1], val[2], val[0]));
  }

  vtkm::cont::DataSet curvi;
  curvi.SetCellSet(recti.GetCellSet());
  curvi.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", sheared));

  return curvi;
}

//-----------------------------------------------------------------------------
class ParametricToWorldCoordinates : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                FieldInOutCell pcs,
                                FieldOutCell wcs);
  using ExecutionSignature = void(CellShape, _2, _3, _4);

  using ScatterType = vtkm::worklet::ScatterPermutation<>;

  VTKM_CONT
  static ScatterType MakeScatter(const vtkm::cont::ArrayHandle<vtkm::Id>& cellIds)
  {
    return ScatterType(cellIds);
  }

  template <typename CellShapeTagType, typename PointsVecType>
  VTKM_EXEC void operator()(CellShapeTagType cellShape,
                            PointsVecType points,
                            const PointType& pc,
                            PointType& wc) const
  {
    auto status = vtkm::exec::CellInterpolate(points, pc, cellShape, wc);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
    }
  }
};

void GenerateRandomInput(const vtkm::cont::DataSet& ds,
                         vtkm::Id count,
                         vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
                         vtkm::cont::ArrayHandle<PointType>& pcoords,
                         vtkm::cont::ArrayHandle<PointType>& wcoords)
{
  vtkm::Id numberOfCells = ds.GetNumberOfCells();

  std::uniform_int_distribution<vtkm::Id> cellIdGen(0, numberOfCells - 1);
  std::uniform_real_distribution<vtkm::FloatDefault> pcoordGen(0.0f, 1.0f);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  auto cwp = cellIds.WritePortal();
  auto pwp = pcoords.WritePortal();
  for (vtkm::Id i = 0; i < count; ++i)
  {
    cwp.Set(i, cellIdGen(RandomGenerator));

    PointType pc{ pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator) };
    pwp.Set(i, pc);
  }

  vtkm::worklet::DispatcherMapTopology<ParametricToWorldCoordinates> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(
    ds.GetCellSet(), ds.GetCoordinateSystem().GetDataAsMultiplexer(), pcoords, wcoords);
}

//-----------------------------------------------------------------------------
class FindCellWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType>
  VTKM_EXEC void operator()(const vtkm::Vec3f& point,
                            const LocatorType& locator,
                            vtkm::Id& cellId,
                            vtkm::Vec3f& pcoords) const
  {
    vtkm::ErrorCode status = locator.FindCell(point, cellId, pcoords);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
    }
  }
};

template <typename CellSetType, typename CoordinateSystemArrayType>
void TestWithDataSet(const vtkm::cont::DataSet& dataset)
{
  VTKM_TEST_ASSERT(dataset.GetCellSet().IsType<CellSetType>());
  VTKM_TEST_ASSERT(dataset.GetCoordinateSystem().GetData().IsType<CoordinateSystemArrayType>());

  vtkm::cont::CellLocatorChooser<CellSetType, CoordinateSystemArrayType> locator;
  locator.SetCellSet(dataset.GetCellSet());
  locator.SetCoordinates(dataset.GetCoordinateSystem());
  locator.Update();

  vtkm::cont::ArrayHandle<vtkm::Id> expCellIds;
  vtkm::cont::ArrayHandle<PointType> expPCoords;
  vtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput(dataset, 32, expCellIds, expPCoords, points);

  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> pcoords;

  vtkm::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
  dispatcher.Invoke(points, locator, cellIds, pcoords);

  auto cellIdPortal = cellIds.ReadPortal();
  auto expCellIdsPortal = expCellIds.ReadPortal();
  auto pcoordsPortal = pcoords.ReadPortal();
  auto expPCoordsPortal = expPCoords.ReadPortal();
  for (vtkm::Id i = 0; i < 32; ++i)
  {
    VTKM_TEST_ASSERT(cellIdPortal.Get(i) == expCellIdsPortal.Get(i), "Incorrect cell ids");
    VTKM_TEST_ASSERT(test_equal(pcoordsPortal.Get(i), expPCoordsPortal.Get(i), 1e-3),
                     "Incorrect parameteric coordinates");
  }
}

void TestCellLocatorChooser()
{
  TestWithDataSet<vtkm::cont::CellSetStructured<3>, vtkm::cont::ArrayHandleUniformPointCoordinates>(
    MakeTestDataSetUniform());

  TestWithDataSet<
    vtkm::cont::CellSetStructured<3>,
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>(
    MakeTestDataSetRectilinear());

  TestWithDataSet<vtkm::cont::CellSetStructured<3>, vtkm::cont::ArrayHandle<PointType>>(
    MakeTestDataSetCurvilinear());
}

} // anonymous namespace

int UnitTestCellLocatorChooser(int argc, char* argv[])
{
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagSerial{});
  return vtkm::cont::testing::Testing::Run(TestCellLocatorChooser, argc, argv);
}
