//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/ThresholdPoints.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// Predicate for values less than minimum
class ValuesBelow
{
public:
  VTKM_CONT
  ValuesBelow(const vtkm::Float32& thresholdValue) :
    ThresholdValue(thresholdValue)
  { }

  template<typename T>
  VTKM_EXEC
  bool operator()(const T& value) const
  {
    return value <= static_cast<T>(this->ThresholdValue);
  }

private:
  vtkm::Float32 ThresholdValue;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  VTKM_CONT
  ValuesAbove(const vtkm::Float32& thresholdValue) :
    ThresholdValue(thresholdValue)
  { }

  template<typename T>
  VTKM_EXEC
  bool operator()(const T& value) const
  {
    return value >= static_cast<T>(this->ThresholdValue);
  }

private:
  vtkm::Float32 ThresholdValue;
};


// Predicate for values between minimum and maximum
class ValuesBetween
{
public:
  VTKM_CONT
  ValuesBetween(const vtkm::Float64& lower,
                const vtkm::Float64& upper) :
    Lower(lower),
    Upper(upper)
  { }

  template<typename T>
  VTKM_EXEC
  bool operator()(const T& value) const
  {
    return value >= static_cast<T>(this->Lower) &&
           value <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

using vtkm::cont::testing::MakeTestDataSet;

template <typename DeviceAdapter>
class TestingThresholdPoints
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing threshold on 2D uniform dataset" << std::endl;

    typedef vtkm::cont::CellSetStructured<2> CellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    CellSetType cellset;
    dataset.GetCellSet(0).CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
    dataset.GetField("pointvar").GetData().CopyTo(fieldArray);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    for (vtkm::Id indx = 0; indx < dataset.GetNumberOfFields(); indx++)
    {
      vtkm::cont::Field field = dataset.GetField(indx);
      if (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
      {
        outDataSet.AddField(field);
      }
    }

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet = threshold.Run(cellset,
                               fieldArray,
                               ValuesBetween(40.0f, 71.0f),
                               DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 11), "Wrong result for ThresholdPoints");

    vtkm::cont::Field pointField = outDataSet.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(12) == 50.0f, "Wrong point field data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing threshold on 3D uniform dataset" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    CellSetType cellset;
    dataset.GetCellSet(0).CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
    dataset.GetField("pointvar").GetData().CopyTo(fieldArray);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    for (vtkm::Id indx = 0; indx < dataset.GetNumberOfFields(); indx++)
    {
      vtkm::cont::Field field = dataset.GetField(indx);
      if (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
      {
        outDataSet.AddField(field);
      }
    }

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet = threshold.Run(cellset,
                               fieldArray,
                               ValuesAbove(1.0f),
                               DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27), "Wrong result for ThresholdPoints");

    vtkm::cont::Field pointField = outDataSet.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(31) == 99.0f, "Wrong point field data");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    CellSetType cellset;
    dataset.GetCellSet(0).CopyTo(cellset);

    vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
    dataset.GetField("pointvar").GetData().CopyTo(fieldArray);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    for (vtkm::Id indx = 0; indx < dataset.GetNumberOfFields(); indx++)
    {
      vtkm::cont::Field field = dataset.GetField(indx);
      if (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
      {
        outDataSet.AddField(field);
      }
    }

    // Output dataset gets new cell set of points that meet threshold predicate
    vtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet = threshold.Run(cellset,
                               fieldArray,
                               ValuesBelow(50.0f),
                               DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 6), "Wrong result for ThresholdPoints");

    vtkm::cont::Field pointField = outDataSet.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(3) == 40.2f, "Wrong point field data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};

}

int UnitTestThresholdPoints(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
    TestingThresholdPoints<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
