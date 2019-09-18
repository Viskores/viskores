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
#include <vtkm/worklet/Triangulate.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

class TestingTriangulate
{
public:
  void TestStructured() const
  {
    std::cout << "Testing TriangulateStructured:" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<2>;
    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    // Convert uniform quadrilaterals to triangles
    vtkm::worklet::Triangulate triangulate;
    OutCellSetType outCellSet = triangulate.Run(cellSet);

    // Create the output dataset and assign the input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    // Two triangles are created for every quad cell
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 2),
                     "Wrong result for Triangulate filter");
  }

  void TestExplicit() const
  {
    std::cout << "Testing TriangulateExplicit:" << std::endl;
    using CellSetType = vtkm::cont::CellSetExplicit<>;
    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DExplicitDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> outCellsPerCell;

    // Convert explicit cells to triangles
    vtkm::worklet::Triangulate triangulate;
    OutCellSetType outCellSet = triangulate.Run(cellSet);

    // Create the output dataset explicit cell set with same coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 14),
                     "Wrong result for Triangulate filter");
  }

  void operator()() const
  {
    TestStructured();
    TestExplicit();
  }
};

int UnitTestTriangulate(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingTriangulate(), argc, argv);
}
