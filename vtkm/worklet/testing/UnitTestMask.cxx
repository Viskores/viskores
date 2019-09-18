//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/Mask.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

using vtkm::cont::testing::MakeTestDataSet;

class TestingMask
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniform2D() const
  {
    std::cout << "Testing mask cells structured:" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<2>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Output data set permutation
    vtkm::worklet::Mask maskCells;
    OutCellSetType outCellSet = maskCells.Run(cellSet, 2);

    vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = maskCells.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for Mask");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(7) == 14.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniform3D() const
  {
    std::cout << "Testing mask cells structured:" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<3>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;
    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Output data set with cell set permuted
    vtkm::worklet::Mask maskCells;
    OutCellSetType outCellSet = maskCells.Run(cellSet, 9);

    vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = maskCells.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 7), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 7 &&
                       cellFieldArray.GetPortalConstControl().Get(2) == 18.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicit() const
  {
    std::cout << "Testing mask cells explicit:" << std::endl;

    using CellSetType = vtkm::cont::CellSetExplicit<>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Output data set with cell set permuted
    vtkm::worklet::Mask maskCells;
    OutCellSetType outCellSet = maskCells.Run(cellSet, 2);

    vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = maskCells.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 2), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit();
  }
};

int UnitTestMask(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingMask(), argc, argv);
}
