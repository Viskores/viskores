//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/testing/Testing.h>

// Make sure deprecated types still work (while applicable)
VTKM_DEPRECATED_SUPPRESS_BEGIN
VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetStructured<3>::
                  ExecConnectivityType<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>,
                typename vtkm::cont::CellSetStructured<3>::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagPoint,
                  vtkm::TopologyElementTagCell>::ExecObjectType>::value));
VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetStructured<3>::
                  ExecConnectivityType<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>,
                typename vtkm::cont::CellSetStructured<3>::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagCell,
                  vtkm::TopologyElementTagPoint>::ExecObjectType>::value));

VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetExplicit<>::
                  ExecConnectivityType<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>,
                typename vtkm::cont::CellSetExplicit<>::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagPoint,
                  vtkm::TopologyElementTagCell>::ExecObjectType>::value));
VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetExplicit<>::
                  ExecConnectivityType<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>,
                typename vtkm::cont::CellSetExplicit<>::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagCell,
                  vtkm::TopologyElementTagPoint>::ExecObjectType>::value));

VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetSingleType<>::
                  ExecConnectivityType<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>,
                typename vtkm::cont::CellSetSingleType<>::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagPoint,
                  vtkm::TopologyElementTagCell>::ExecObjectType>::value));
VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetSingleType<>::
                  ExecConnectivityType<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>,
                typename vtkm::cont::CellSetSingleType<>::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagCell,
                  vtkm::TopologyElementTagPoint>::ExecObjectType>::value));

VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>>::
                  ExecConnectivityType<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>,
                typename vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>>::
                  ExecutionTypes<vtkm::cont::DeviceAdapterTagSerial,
                                 vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ExecObjectType>::value));
VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>>::
                  ExecConnectivityType<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>,
                typename vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>>::
                  ExecutionTypes<vtkm::cont::DeviceAdapterTagSerial,
                                 vtkm::TopologyElementTagCell,
                                 vtkm::TopologyElementTagPoint>::ExecObjectType>::value));

VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetExtrude::
                  ExecConnectivityType<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>,
                typename vtkm::cont::CellSetExtrude::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagPoint,
                  vtkm::TopologyElementTagCell>::ExecObjectType>::value));
VTKM_STATIC_ASSERT(
  (std::is_same<typename vtkm::cont::CellSetExtrude::
                  ExecConnectivityType<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>,
                typename vtkm::cont::CellSetExtrude::ExecutionTypes<
                  vtkm::cont::DeviceAdapterTagSerial,
                  vtkm::TopologyElementTagCell,
                  vtkm::TopologyElementTagPoint>::ExecObjectType>::value));
VTKM_DEPRECATED_SUPPRESS_END

namespace
{

constexpr vtkm::Id xdim = 3, ydim = 5, zdim = 7;
constexpr vtkm::Id3 BaseLinePointDimensions{ xdim, ydim, zdim };
constexpr vtkm::Id BaseLineNumberOfPoints = xdim * ydim * zdim;
constexpr vtkm::Id BaseLineNumberOfCells = (xdim - 1) * (ydim - 1) * (zdim - 1);

vtkm::cont::CellSetStructured<3> BaseLine;

void InitializeBaseLine()
{
  BaseLine.SetPointDimensions(BaseLinePointDimensions);
}

class BaseLineConnectivityFunctor
{
public:
  explicit BaseLineConnectivityFunctor()
  {
    this->Structure.SetPointDimensions(BaseLinePointDimensions);
  }

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id idx) const
  {
    auto i = idx / this->Structure.NUM_POINTS_IN_CELL;
    auto c = static_cast<vtkm::IdComponent>(idx % this->Structure.NUM_POINTS_IN_CELL);
    return this->Structure.GetPointsOfCell(i)[c];
  }

private:
  vtkm::internal::ConnectivityStructuredInternals<3> Structure;
};

using BaseLineConnectivityType = vtkm::cont::ArrayHandleImplicit<BaseLineConnectivityFunctor>;
BaseLineConnectivityType BaseLineConnectivity(BaseLineConnectivityFunctor{},
                                              BaseLineNumberOfCells * 8);

auto PermutationArray = vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 2, BaseLineNumberOfCells / 2);

//-----------------------------------------------------------------------------
vtkm::cont::CellSetExplicit<> MakeCellSetExplicit()
{
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  shapes.AllocateAndFill(BaseLineNumberOfCells, vtkm::CELL_SHAPE_HEXAHEDRON);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  numIndices.AllocateAndFill(BaseLineNumberOfCells, 8);

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  vtkm::cont::ArrayCopyDevice(BaseLineConnectivity, connectivity);

  auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numIndices);

  vtkm::cont::CellSetExplicit<> cellset;
  cellset.Fill(BaseLineNumberOfPoints, shapes, connectivity, offsets);
  return cellset;
}

vtkm::cont::CellSetSingleType<typename BaseLineConnectivityType::StorageTag> MakeCellSetSingleType()
{
  vtkm::cont::CellSetSingleType<typename BaseLineConnectivityType::StorageTag> cellset;
  cellset.Fill(BaseLineNumberOfPoints, vtkm::CELL_SHAPE_HEXAHEDRON, 8, BaseLineConnectivity);
  return cellset;
}

vtkm::cont::CellSetStructured<3> MakeCellSetStructured()
{
  vtkm::cont::CellSetStructured<3> cellset;
  cellset.SetPointDimensions(BaseLinePointDimensions);
  return cellset;
}

//-----------------------------------------------------------------------------
enum class IsPermutationCellSet
{
  NO = 0,
  YES = 1
};

void TestAgainstBaseLine(const vtkm::cont::CellSet& cellset,
                         IsPermutationCellSet flag = IsPermutationCellSet::NO)
{
  vtkm::internal::ConnectivityStructuredInternals<3> baseLineStructure;
  baseLineStructure.SetPointDimensions(BaseLinePointDimensions);

  VTKM_TEST_ASSERT(cellset.GetNumberOfPoints() == BaseLineNumberOfPoints, "Wrong number of points");

  vtkm::Id numCells = cellset.GetNumberOfCells();
  vtkm::Id expectedNumCell = (flag == IsPermutationCellSet::NO)
    ? BaseLineNumberOfCells
    : PermutationArray.GetNumberOfValues();
  VTKM_TEST_ASSERT(numCells == expectedNumCell, "Wrong number of cells");

  auto permutationPortal = PermutationArray.ReadPortal();
  for (vtkm::Id i = 0; i < numCells; ++i)
  {
    VTKM_TEST_ASSERT(cellset.GetCellShape(i) == vtkm::CELL_SHAPE_HEXAHEDRON, "Wrong shape");
    VTKM_TEST_ASSERT(cellset.GetNumberOfPointsInCell(i) == 8, "Wrong number of points-of-cell");

    vtkm::Id baseLineCellId = (flag == IsPermutationCellSet::YES) ? permutationPortal.Get(i) : i;
    auto baseLinePointIds = baseLineStructure.GetPointsOfCell(baseLineCellId);

    vtkm::Id pointIds[8];
    cellset.GetCellPointIds(i, pointIds);
    for (int j = 0; j < 8; ++j)
    {
      VTKM_TEST_ASSERT(pointIds[j] == baseLinePointIds[j], "Wrong points-of-cell point id");
    }
  }
}

void RunTests(const vtkm::cont::CellSet& cellset,
              IsPermutationCellSet flag = IsPermutationCellSet::NO)
{
  TestAgainstBaseLine(cellset, flag);
  auto deepcopy = cellset.NewInstance();
  deepcopy->DeepCopy(&cellset);
  TestAgainstBaseLine(*deepcopy, flag);
}

void TestCellSet()
{
  InitializeBaseLine();

  std::cout << "Testing CellSetExplicit\n";
  auto csExplicit = MakeCellSetExplicit();
  RunTests(csExplicit);
  std::cout << "Testing CellSetPermutation of CellSetExplicit\n";
  RunTests(vtkm::cont::make_CellSetPermutation(PermutationArray, csExplicit),
           IsPermutationCellSet::YES);

  std::cout << "Testing CellSetSingleType\n";
  auto csSingle = MakeCellSetSingleType();
  RunTests(csSingle);
  std::cout << "Testing CellSetPermutation of CellSetSingleType\n";
  RunTests(vtkm::cont::make_CellSetPermutation(PermutationArray, csSingle),
           IsPermutationCellSet::YES);

  std::cout << "Testing CellSetStructured\n";
  auto csStructured = MakeCellSetStructured();
  RunTests(csStructured);
  std::cout << "Testing CellSetPermutation of CellSetStructured\n";
  RunTests(vtkm::cont::make_CellSetPermutation(PermutationArray, csStructured),
           IsPermutationCellSet::YES);
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestCellSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSet, argc, argv);
}
