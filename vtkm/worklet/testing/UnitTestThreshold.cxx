//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/Threshold.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{

class HasValue
{
public:
  VTKM_CONT
  HasValue(vtkm::Float32 value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(ScalarType value) const
  {
    return static_cast<vtkm::Float32>(value) == this->Value;
  }

private:
  vtkm::Float32 Value;
};

using vtkm::cont::testing::MakeTestDataSet;

class TestingThreshold
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing threshold on 2D uniform dataset" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<2>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> pointvar;
    dataset.GetField("pointvar").GetData().CopyTo(pointvar);

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, pointvar, vtkm::cont::Field::Association::POINTS, HasValue(60.1f));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 200.1f,
                     "Wrong cell field data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing threshold on 3D uniform dataset" << std::endl;

    using CellSetType = vtkm::cont::CellSetStructured<3>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> pointvar;
    dataset.GetField("pointvar").GetData().CopyTo(pointvar);

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, pointvar, vtkm::cont::Field::Association::POINTS, HasValue(20.1f));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 2, "Wrong number of cells");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;

    using CellSetType = vtkm::cont::CellSetExplicit<>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, cellvar, vtkm::cont::Field::Association::CELL_SET, HasValue(100.1f));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 100.1f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};
}

int UnitTestThreshold(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThreshold(), argc, argv);
}
