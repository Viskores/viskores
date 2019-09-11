//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellLocatorGeneral.h>

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
    vtkm::Id3{ 64 }, PointType{ -32.0f }, PointType{ 1.0f / 64.0f });
}

vtkm::cont::DataSet MakeTestDataSetRectilinear()
{
  std::uniform_real_distribution<vtkm::FloatDefault> coordGen(1.0f / 128.0f, 1.0f / 32.0f);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> coords[3];
  for (int i = 0; i < 3; ++i)
  {
    coords[i].Allocate(64);
    auto portal = coords[i].GetPortalControl();

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
  auto coords = recti.GetCoordinateSystem().GetData();

  vtkm::cont::ArrayHandle<PointType> sheared;
  sheared.Allocate(coords.GetNumberOfValues());

  auto inPortal = coords.GetPortalConstControl();
  auto outPortal = sheared.GetPortalControl();
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
    wc = vtkm::exec::CellInterpolate(points, pc, cellShape, *this);
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

  for (vtkm::Id i = 0; i < count; ++i)
  {
    cellIds.GetPortalControl().Set(i, cellIdGen(RandomGenerator));

    PointType pc{ pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator) };
    pcoords.GetPortalControl().Set(i, pc);
  }

  vtkm::worklet::DispatcherMapTopology<ParametricToWorldCoordinates> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(ds.GetCellSet(), ds.GetCoordinateSystem().GetData(), pcoords, wcoords);
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
    locator->FindCell(point, cellId, pcoords, *this);
  }
};

void TestWithDataSet(vtkm::cont::CellLocatorGeneral& locator, const vtkm::cont::DataSet& dataset)
{
  locator.SetCellSet(dataset.GetCellSet());
  locator.SetCoordinates(dataset.GetCoordinateSystem());
  locator.Update();

  const vtkm::cont::CellLocator& curLoc = *locator.GetCurrentLocator();
  std::cout << "using locator: " << typeid(curLoc).name() << "\n";

  vtkm::cont::ArrayHandle<vtkm::Id> expCellIds;
  vtkm::cont::ArrayHandle<PointType> expPCoords;
  vtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput(dataset, 128, expCellIds, expPCoords, points);

  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> pcoords;

  vtkm::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
  // CellLocatorGeneral is non-copyable. Pass it via a pointer.
  dispatcher.Invoke(points, &locator, cellIds, pcoords);

  for (vtkm::Id i = 0; i < 128; ++i)
  {
    VTKM_TEST_ASSERT(cellIds.GetPortalConstControl().Get(i) ==
                       expCellIds.GetPortalConstControl().Get(i),
                     "Incorrect cell ids");
    VTKM_TEST_ASSERT(test_equal(pcoords.GetPortalConstControl().Get(i),
                                expPCoords.GetPortalConstControl().Get(i),
                                1e-3),
                     "Incorrect parameteric coordinates");
  }

  std::cout << "Passed.\n";
}

void TestCellLocatorGeneral()
{
  vtkm::cont::CellLocatorGeneral locator;

  std::cout << "Test UniformGrid dataset\n";
  TestWithDataSet(locator, MakeTestDataSetUniform());

  std::cout << "Test Rectilinear dataset\n";
  TestWithDataSet(locator, MakeTestDataSetRectilinear());

  std::cout << "Test Curvilinear dataset\n";
  TestWithDataSet(locator, MakeTestDataSetCurvilinear());
}

} // anonymous namespace

int UnitTestCellLocatorGeneral(int argc, char* argv[])
{
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagSerial{});
  return vtkm::cont::testing::Testing::Run(TestCellLocatorGeneral, argc, argv);
}
