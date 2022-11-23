//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/filter/geometry_refinement/worklet/Tetrahedralize.h>
#include <vtkm/filter/geometry_refinement/worklet/Triangulate.h>
#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/CellShape.h>

#include <ctime>
#include <random>

namespace
{

using PointType = vtkm::Vec3f;

std::default_random_engine RandomGenerator;

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
    auto status = vtkm::exec::ParametricCoordinatesToWorldCoordinates(points, pc, cellShape, wc);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
    }
  }
};

template <vtkm::IdComponent DIMENSIONS>
vtkm::cont::DataSet MakeTestDataSet(const vtkm::Vec<vtkm::Id, DIMENSIONS>& dims)
{
  auto uniformDs =
    vtkm::cont::DataSetBuilderUniform::Create(dims,
                                              vtkm::Vec<vtkm::FloatDefault, DIMENSIONS>(0.0f),
                                              vtkm::Vec<vtkm::FloatDefault, DIMENSIONS>(1.0f));

  auto uniformCs =
    uniformDs.GetCellSet().template AsCellSet<vtkm::cont::CellSetStructured<DIMENSIONS>>();

  // triangulate the cellset
  vtkm::cont::CellSetSingleType<> cellset;
  switch (DIMENSIONS)
  {
    case 2:
      cellset = vtkm::worklet::Triangulate().Run(uniformCs);
      break;
    case 3:
      cellset = vtkm::worklet::Tetrahedralize().Run(uniformCs);
      break;
    default:
      VTKM_ASSERT(false);
  }

  // Warp the coordinates
  std::uniform_real_distribution<vtkm::FloatDefault> warpFactor(-0.10f, 0.10f);
  auto inPointsPortal = uniformDs.GetCoordinateSystem()
                          .GetData()
                          .template AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>()
                          .ReadPortal();
  vtkm::cont::ArrayHandle<PointType> points;
  points.Allocate(inPointsPortal.GetNumberOfValues());
  auto outPointsPortal = points.WritePortal();
  for (vtkm::Id i = 0; i < outPointsPortal.GetNumberOfValues(); ++i)
  {
    PointType warpVec(0);
    for (vtkm::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      warpVec[c] = warpFactor(RandomGenerator);
    }
    outPointsPortal.Set(i, inPointsPortal.Get(i) + warpVec);
  }

  // build dataset
  vtkm::cont::DataSet out;
  out.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", points));
  out.SetCellSet(cellset);
  return out;
}

template <vtkm::IdComponent DIMENSIONS>
void GenerateRandomInput(const vtkm::cont::DataSet& ds,
                         vtkm::Id count,
                         vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
                         vtkm::cont::ArrayHandle<PointType>& pcoords,
                         vtkm::cont::ArrayHandle<PointType>& wcoords)
{
  vtkm::Id numberOfCells = ds.GetNumberOfCells();

  std::uniform_int_distribution<vtkm::Id> cellIdGen(0, numberOfCells - 1);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  for (vtkm::Id i = 0; i < count; ++i)
  {
    cellIds.WritePortal().Set(i, cellIdGen(RandomGenerator));

    PointType pc(0.0f);
    vtkm::FloatDefault minPc = 1e-2f;
    vtkm::FloatDefault sum = 0.0f;
    for (vtkm::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      vtkm::FloatDefault maxPc =
        1.0f - (static_cast<vtkm::FloatDefault>(DIMENSIONS - c) * minPc) - sum;
      std::uniform_real_distribution<vtkm::FloatDefault> pcoordGen(minPc, maxPc);
      pc[c] = pcoordGen(RandomGenerator);
      sum += pc[c];
    }
    pcoords.WritePortal().Set(i, pc);
  }

  vtkm::cont::Invoker invoker;
  invoker(ParametricToWorldCoordinates{},
          ParametricToWorldCoordinates::MakeScatter(cellIds),
          ds.GetCellSet(),
          ds.GetCoordinateSystem().GetDataAsMultiplexer(),
          pcoords,
          wcoords);
}

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

class FindCellWorkletWithLastCell : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords,
                                FieldInOut lastCell);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename LocatorType>
  VTKM_EXEC void operator()(const vtkm::Vec3f& point,
                            const LocatorType& locator,
                            vtkm::Id& cellId,
                            vtkm::Vec3f& pcoords,
                            typename LocatorType::LastCell& lastCell) const
  {
    vtkm::ErrorCode status = locator.FindCell(point, cellId, pcoords, lastCell);
    if (status != vtkm::ErrorCode::Success)
      this->RaiseError(vtkm::ErrorString(status));
  }
};

template <typename LocatorType>
void TestLastCell(LocatorType& locator,
                  vtkm::Id numPoints,
                  vtkm::cont::ArrayHandle<typename LocatorType::LastCell>& lastCell,
                  const vtkm::cont::ArrayHandle<PointType>& points,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& expCellIds,
                  const vtkm::cont::ArrayHandle<PointType>& expPCoords)

{
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> pcoords;

  vtkm::cont::Invoker invoker;
  invoker(FindCellWorkletWithLastCell{}, points, locator, cellIds, pcoords, lastCell);

  auto cellIdPortal = cellIds.ReadPortal();
  auto expCellIdsPortal = expCellIds.ReadPortal();
  auto pcoordsPortal = pcoords.ReadPortal();
  auto expPCoordsPortal = expPCoords.ReadPortal();

  for (vtkm::Id i = 0; i < numPoints; ++i)
  {
    VTKM_TEST_ASSERT(cellIdPortal.Get(i) == expCellIdsPortal.Get(i), "Incorrect cell ids");
    VTKM_TEST_ASSERT(test_equal(pcoordsPortal.Get(i), expPCoordsPortal.Get(i), 1e-3),
                     "Incorrect parameteric coordinates");
  }
}

template <typename LocatorType, vtkm::IdComponent DIMENSIONS>
void TestCellLocator(LocatorType& locator,
                     const vtkm::Vec<vtkm::Id, DIMENSIONS>& dim,
                     vtkm::Id numberOfPoints)
{
  auto ds = MakeTestDataSet(dim);

  std::cout << "Testing " << DIMENSIONS << "D dataset with " << ds.GetNumberOfCells() << " cells\n";

  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Update();

  vtkm::cont::ArrayHandle<vtkm::Id> expCellIds;
  vtkm::cont::ArrayHandle<PointType> expPCoords;
  vtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput<DIMENSIONS>(ds, numberOfPoints, expCellIds, expPCoords, points);

  std::cout << "Finding cells for " << numberOfPoints << " points\n";
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> pcoords;

  vtkm::cont::Invoker invoker;
  invoker(FindCellWorklet{}, points, locator, cellIds, pcoords);

  auto cellIdsPortal = cellIds.ReadPortal();
  auto expCellIdsPortal = expCellIds.ReadPortal();
  auto pcoordsPortal = pcoords.ReadPortal();
  auto expPCoordsPortal = expPCoords.ReadPortal();
  for (vtkm::Id i = 0; i < numberOfPoints; ++i)
  {
    VTKM_TEST_ASSERT(cellIdsPortal.Get(i) == expCellIdsPortal.Get(i), "Incorrect cell ids");
    VTKM_TEST_ASSERT(test_equal(pcoordsPortal.Get(i), expPCoordsPortal.Get(i), 1e-3),
                     "Incorrect parameteric coordinates");
  }

  //Test locator using lastCell

  //Test it with initialized.
  vtkm::cont::ArrayHandle<typename LocatorType::LastCell> lastCell;
  lastCell.AllocateAndFill(numberOfPoints, typename LocatorType::LastCell{});
  TestLastCell(locator, numberOfPoints, lastCell, points, expCellIds, pcoords);

  //Call it again using the lastCell just computed to validate.
  TestLastCell(locator, numberOfPoints, lastCell, points, expCellIds, pcoords);

  //Test it with uninitialized array.
  vtkm::cont::ArrayHandle<typename LocatorType::LastCell> lastCell2;
  lastCell2.Allocate(numberOfPoints);
  TestLastCell(locator, numberOfPoints, lastCell2, points, expCellIds, pcoords);

  //Call it again using the lastCell2 just computed to validate.
  TestLastCell(locator, numberOfPoints, lastCell2, points, expCellIds, pcoords);
}

void TestingCellLocatorUnstructured()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  RandomGenerator.seed(seed);

  //Test vtkm::cont::CellLocatorTwoLevel
  vtkm::cont::CellLocatorTwoLevel locator2L;
  locator2L.SetDensityL1(64.0f);
  locator2L.SetDensityL2(1.0f);

  TestCellLocator(locator2L, vtkm::Id3(8), 512);  // 3D dataset
  TestCellLocator(locator2L, vtkm::Id2(18), 512); // 2D dataset

  //Test vtkm::cont::CellLocatorUniformBins
  vtkm::cont::CellLocatorUniformBins locatorUB;
  locatorUB.SetDims({ 32, 32, 32 });
  TestCellLocator(locatorUB, vtkm::Id3(8), 512);  // 3D dataset
  TestCellLocator(locatorUB, vtkm::Id2(18), 512); // 2D dataset

  //Test 2D dataset with 2D bins.
  locatorUB.SetDims({ 32, 32, 1 });
  TestCellLocator(locatorUB, vtkm::Id2(18), 512); // 2D dataset
}


} // anonymous

int UnitTestCellLocatorUnstructured(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingCellLocatorUnstructured, argc, argv);
}
