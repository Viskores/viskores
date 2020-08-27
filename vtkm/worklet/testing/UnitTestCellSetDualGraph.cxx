//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/Contour.h>

#include <vtkm/worklet/connectivities/CellSetDualGraph.h>

class TestCellSetDualGraph
{
private:
  using GroupedConnectivityArrayType =
    vtkm::cont::ArrayHandleGroupVecVariable<vtkm::cont::ArrayHandle<vtkm::Id>,
                                            vtkm::cont::ArrayHandle<vtkm::Id>>;

  static GroupedConnectivityArrayType MakeGroupedConnectivity(
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity,
    vtkm::cont::ArrayHandle<vtkm::Id> counts)
  {
    return GroupedConnectivityArrayType(connectivity,
                                        vtkm::cont::ConvertNumComponentsToOffsets(counts));
  }

  static bool TestConnectivity(GroupedConnectivityArrayType computedConnectivityArray,
                               GroupedConnectivityArrayType expectedConnectivityArray)
  {
    auto computedConnections = computedConnectivityArray.ReadPortal();
    auto expectedConnections = expectedConnectivityArray.ReadPortal();

    vtkm::Id numItems = computedConnections.GetNumberOfValues();
    if (numItems != expectedConnections.GetNumberOfValues())
    {
      return false;
    }

    for (vtkm::Id itemIndex = 0; itemIndex < numItems; ++itemIndex)
    {
      auto computed = computedConnections.Get(itemIndex);
      auto expected = expectedConnections.Get(itemIndex);
      vtkm::IdComponent numConnections = computed.GetNumberOfComponents();
      if (numConnections != expected.GetNumberOfComponents())
      {
        return false;
      }

      // computed and expected are Vec-like objects that should represent the same thing.
      // However, although both should have the same indices, they may be in different
      // orders.
      std::set<vtkm::Id> computedSet;
      std::set<vtkm::Id> expectedSet;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < numConnections; ++componentIndex)
      {
        computedSet.insert(computed[componentIndex]);
        expectedSet.insert(expected[componentIndex]);
      }
      if (computedSet != expectedSet)
      {
        return false;
      }
    }

    return true;
  }

public:
  void TestTriangleMesh() const
  {
    auto connectivity = vtkm::cont::make_ArrayHandle<vtkm::Id>(
      { 0, 2, 4, 1, 3, 5, 2, 6, 4, 5, 3, 7, 2, 9, 6, 4, 6, 8 });

    vtkm::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(10, vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);

    vtkm::cont::ArrayHandle<vtkm::Id> numIndicesArray;
    vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetArray;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;

    vtkm::worklet::connectivity::CellSetDualGraph().Run(
      cellSet, numIndicesArray, indexOffsetArray, connectivityArray);

    vtkm::cont::ArrayHandle<vtkm::Id> expectedNumIndices =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 1, 3, 1, 1, 1 });
    VTKM_TEST_ASSERT(
      test_equal_portals(numIndicesArray.ReadPortal(), expectedNumIndices.ReadPortal()));

    vtkm::cont::ArrayHandle<vtkm::Id> expectedIndexOffset =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 5, 6, 7 });
    VTKM_TEST_ASSERT(
      test_equal_portals(indexOffsetArray.ReadPortal(), expectedIndexOffset.ReadPortal()));

    vtkm::cont::ArrayHandle<vtkm::Id> expectedConnectivity =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 2, 3, 0, 4, 5, 1, 2, 2 });
    VTKM_TEST_ASSERT(
      TestConnectivity(MakeGroupedConnectivity(connectivityArray, numIndicesArray),
                       MakeGroupedConnectivity(expectedConnectivity, numIndicesArray)));
  }

  void operator()() const { this->TestTriangleMesh(); }
};

int UnitTestCellSetDualGraph(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetDualGraph(), argc, argv);
}
