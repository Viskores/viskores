//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/Contour.h>

#include <vtkm/worklet/connectivities/CellSetDualGraph.h>

class TestCellSetDualGraph
{
private:
  template <typename T, typename Storage>
  bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage>& ah,
                       const T* expected,
                       vtkm::Id size) const
  {
    if (size != ah.GetNumberOfValues())
    {
      return false;
    }

    for (vtkm::Id i = 0; i < size; ++i)
    {
      if (ah.GetPortalConstControl().Get(i) != expected[i])
      {
        return false;
      }
    }

    return true;
  }

public:
  void TestTriangleMesh() const
  {
    std::vector<vtkm::Id> connectivity = { 0, 2, 4, 1, 3, 5, 2, 6, 4, 5, 3, 7, 2, 9, 6, 4, 6, 8 };

    vtkm::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(8, vtkm::CELL_SHAPE_TRIANGLE, 3, vtkm::cont::make_ArrayHandle(connectivity));

    vtkm::cont::ArrayHandle<vtkm::Id> numIndicesArray;
    vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetArray;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;

    vtkm::worklet::connectivity::CellSetDualGraph().Run(
      cellSet, numIndicesArray, indexOffsetArray, connectivityArray);

    vtkm::Id expectedNumIndices[] = { 1, 1, 3, 1, 1, 1 };
    VTKM_TEST_ASSERT(numIndicesArray.GetNumberOfValues() == 6,
                     "Wrong number of elements in NumIndices array");
    VTKM_TEST_ASSERT(TestArrayHandle(numIndicesArray, expectedNumIndices, 6),
                     "Got incorrect numIndices");

    vtkm::Id expectedIndexOffset[] = { 0, 1, 2, 5, 6, 7 };
    VTKM_TEST_ASSERT(TestArrayHandle(indexOffsetArray, expectedIndexOffset, 6),
                     "Got incorrect indexOffset");

    vtkm::Id expectedConnectivity[] = { 2, 3, 0, 4, 5, 1, 2, 2 };
    VTKM_TEST_ASSERT(TestArrayHandle(connectivityArray, expectedConnectivity, 8),
                     "Got incorrect dual graph connectivity");
  }

  void operator()() const { this->TestTriangleMesh(); }
};

int UnitTestCellSetDualGraph(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetDualGraph(), argc, argv);
}
