//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/Triangulate.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

template <typename DeviceAdapter>
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
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // Convert uniform quadrilaterals to triangles
    vtkm::worklet::Triangulate triangulate;
    OutCellSetType outCellSet = triangulate.Run(cellSet, DeviceAdapter());

    // Create the output dataset and assign the input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.AddCellSet(outCellSet);

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
    dataSet.GetCellSet(0).CopyTo(cellSet);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> outCellsPerCell;

    // Convert explicit cells to triangles
    vtkm::worklet::Triangulate triangulate;
    OutCellSetType outCellSet = triangulate.Run(cellSet, DeviceAdapter());

    // Create the output dataset explicit cell set with same coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 14),
                     "Wrong result for Triangulate filter");
  }

  void operator()() const
  {
    TestStructured();
    TestExplicit();
  }
};

int UnitTestTriangulate(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestingTriangulate<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
