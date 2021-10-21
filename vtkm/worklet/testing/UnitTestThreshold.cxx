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

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <iostream>
#include <vector>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

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

class ThresholdRange
{
public:
  VTKM_CONT
  ThresholdRange() {}

  ThresholdRange(const vtkm::Float64& lower, const vtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  void SetLowerValue(const vtkm::Float64& lower) { Lower = lower; }
  void SetUpperValue(const vtkm::Float64& upper) { Upper = upper; }

  template <typename T>
  VTKM_EXEC bool operator()(const T& value) const
  {

    return value >= static_cast<T>(this->Lower) && value <= static_cast<T>(this->Upper);
  }

  //Needed to work with ArrayHandleVirtual
  template <typename PortalType>
  VTKM_EXEC bool operator()(
    const vtkm::internal::ArrayPortalValueReference<PortalType>& value) const
  {
    using T = typename PortalType::ValueType;
    return value.Get() >= static_cast<T>(this->Lower) && value.Get() <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

using vtkm::cont::testing::MakeTestDataSet;

class TestingThreshold
{
public:
  void TestUniform2D(bool returnAllInRange) const
  {
    using CellSetType = vtkm::cont::CellSetStructured<2>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> pointvar;
    dataset.GetField("pointvar").GetData().AsArrayHandle(pointvar);

    vtkm::worklet::Threshold threshold;
    ThresholdRange predicate;

    if (returnAllInRange)
    {
      std::cout << "Testing threshold on 2D uniform dataset returning values 'all in range'"
                << std::endl;
      predicate.SetLowerValue(10);
      predicate.SetUpperValue(60);
    }
    else
    {
      std::cout << "Testing threshold on 2D uniform dataset returning values 'part in range'"
                << std::endl;
      predicate.SetLowerValue(60);
      predicate.SetUpperValue(61);
    }

    OutCellSetType outCellSet = threshold.Run(
      cellset, pointvar, vtkm::cont::Field::Association::POINTS, predicate, returnAllInRange);

    if (returnAllInRange)
    {
      VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
      dataset.GetField("cellvar").GetData().AsArrayHandle(cellvar);
      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f,
                       "Wrong cell field data");
    }
    else
    {
      VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
      dataset.GetField("cellvar").GetData().AsArrayHandle(cellvar);
      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                         cellFieldArray.ReadPortal().Get(0) == 200.1f,
                       "Wrong cell field data");
    }
  }

  void TestUniform3D(bool returnAllInRange) const
  {
    using CellSetType = vtkm::cont::CellSetStructured<3>;
    using OutCellSetType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> pointvar;
    dataset.GetField("pointvar").GetData().AsArrayHandle(pointvar);

    vtkm::worklet::Threshold threshold;
    ThresholdRange predicate;

    if (returnAllInRange)
    {
      std::cout << "Testing threshold on 3D uniform dataset returning values 'all in range'"
                << std::endl;
      predicate.SetLowerValue(10.1);
      predicate.SetUpperValue(180);
    }
    else
    {
      std::cout << "Testing threshold on 3D uniform dataset returning values 'part in range'"
                << std::endl;
      predicate.SetLowerValue(20);
      predicate.SetUpperValue(21);
    }

    OutCellSetType outCellSet = threshold.Run(
      cellset, pointvar, vtkm::cont::Field::Association::POINTS, predicate, returnAllInRange);

    if (returnAllInRange)
    {
      VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 3, "Wrong number of cells");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
      dataset.GetField("cellvar").GetData().AsArrayHandle(cellvar);
      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 3 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f &&
                         cellFieldArray.ReadPortal().Get(1) == 100.2f &&
                         cellFieldArray.ReadPortal().Get(2) == 100.3f,
                       "Wrong cell field data");
    }
    else
    {
      VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 2, "Wrong number of cells");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellvar;
      dataset.GetField("cellvar").GetData().AsArrayHandle(cellvar);
      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f &&
                         cellFieldArray.ReadPortal().Get(1) == 100.2f,
                       "Wrong cell field data");
    }
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
    dataset.GetField("cellvar").GetData().AsArrayHandle(cellvar);

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, cellvar, vtkm::cont::Field::Association::CELL_SET, HasValue(100.1f));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                       cellFieldArray.ReadPortal().Get(0) == 100.1f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniform2D(false);
    this->TestUniform2D(true);
    this->TestUniform3D(false);
    this->TestUniform3D(true);
    this->TestExplicit3D();
  }
};
}

int UnitTestThreshold(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThreshold(), argc, argv);
}
