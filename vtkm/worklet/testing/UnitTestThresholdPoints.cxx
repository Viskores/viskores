//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ThresholdPoints.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{

// Predicate for values less than minimum
class ValuesBelow
{
public:
  VTKM_CONT
  ValuesBelow(const vtkm::FloatDefault& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return static_cast<vtkm::FloatDefault>(value) <= this->Value;
  }

private:
  vtkm::FloatDefault Value;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  VTKM_CONT
  ValuesAbove(const vtkm::FloatDefault& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return static_cast<vtkm::FloatDefault>(value) >= this->Value;
  }

private:
  vtkm::FloatDefault Value;
};

// Predicate for values between minimum and maximum
class ValuesBetween
{
public:
  VTKM_CONT
  ValuesBetween(const vtkm::FloatDefault& lower, const vtkm::FloatDefault& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return static_cast<vtkm::FloatDefault>(value) >= this->Lower &&
      static_cast<vtkm::FloatDefault>(value) <= this->Upper;
  }

private:
  vtkm::FloatDefault Lower;
  vtkm::FloatDefault Upper;
};

using vtkm::cont::testing::MakeTestDataSet;

class TestingThresholdPoints
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing threshold on 2D uniform dataset" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    outDataSet.AddField(dataset.GetField("pointvar"));

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet =
      threshold.Run(dataset.GetCellSet(),
                    dataset.GetField("pointvar").GetData().ResetTypes(vtkm::TypeListFieldScalar()),
                    ValuesBetween(40.0f, 71.0f));
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 11),
                     "Wrong result for ThresholdPoints");

    vtkm::cont::Field pointField = outDataSet.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(12) == 50.0f,
                     "Wrong point field data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing threshold on 3D uniform dataset" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    outDataSet.AddField(dataset.GetField("pointvar"));

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet =
      threshold.Run(dataset.GetCellSet(),
                    dataset.GetField("pointvar").GetData().ResetTypes(vtkm::TypeListFieldScalar()),
                    ValuesAbove(1.0f));
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27),
                     "Wrong result for ThresholdPoints");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet =
      threshold.Run(dataset.GetCellSet(),
                    dataset.GetField("pointvar").GetData().ResetTypes(vtkm::TypeListFieldScalar()),
                    ValuesBelow(50.0f));
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 6),
                     "Wrong result for ThresholdPoints");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};
}

int UnitTestThresholdPoints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThresholdPoints(), argc, argv);
}
