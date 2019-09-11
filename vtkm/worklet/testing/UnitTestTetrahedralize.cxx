//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/Tetrahedralize.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

class TestingTetrahedralize
{
public:
  //
  // Create a uniform 3D structured cell set as input
  // Add a field which is the index type which is (i+j+k) % 2 to alternate tetrahedralization pattern
  // Points are all the same, but each hexahedron cell becomes 5 tetrahedral cells
  //
  void TestStructured() const
  {
    std::cout << "Testing TetrahedralizeStructured" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;
    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    // Convert uniform hexahedra to tetrahedra
    vtkm::worklet::Tetrahedralize tetrahedralize;
    OutCellSetType outCellSet = tetrahedralize.Run(cellSet);

    // Create the output dataset with same coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 5),
                     "Wrong result for Tetrahedralize filter");
  }

  //
  // Create an explicit 3D cell set as input and fill
  // Points are all the same, but each cell becomes tetrahedra
  //
  void TestExplicit() const
  {
    std::cout << "Testing TetrahedralizeExplicit" << std::endl;
    using CellSetType = vtkm::cont::CellSetExplicit<>;
    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Create the input explicit cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> outCellsPerCell;

    // Convert explicit cells to tetrahedra
    vtkm::worklet::Tetrahedralize tetrahedralize;
    OutCellSetType outCellSet = tetrahedralize.Run(cellSet);

    // Create the output dataset explicit cell set with same coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 11),
                     "Wrong result for Tetrahedralize filter");
  }

  void operator()() const
  {
    TestStructured();
    TestExplicit();
  }
};

int UnitTestTetrahedralize(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingTetrahedralize(), argc, argv);
}
