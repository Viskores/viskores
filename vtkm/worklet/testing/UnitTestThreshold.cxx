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

#include <vtkm/worklet/Threshold.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

class HasValue
{
public:
  VTKM_CONT_EXPORT
  HasValue(vtkm::Float32 value) : Value(value)
  { }

  template <typename ScalarType>
  VTKM_EXEC_EXPORT
  bool operator()(ScalarType value) const
  {
    return static_cast<vtkm::Float32>(value) == this->Value;
  }

private:
  vtkm::Float32 Value;
};

using vtkm::cont::testing::MakeTestDataSet;

template <typename DeviceAdapter>
class TestingThreshold
{
public:
  void TestRegular2D() const
  {
    std::cout << "Testing threshold on 2D regular dataset" << std::endl;

    typedef vtkm::cont::CellSetStructured<2> CellSetType;
    typedef vtkm::cont::CellSetPermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                           CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DRegularDataSet0();

    CellSetType cellset = dataset.GetCellSet(0).CastTo(CellSetType());

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet = threshold.Run(cellset,
                                              dataset.GetField("pointvar"),
                                              HasValue(60.1f),
                                              DeviceAdapter());
    vtkm::cont::Field cellField =
        threshold.ProcessCellField(dataset.GetField("cellvar"));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CastToArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 200.1f,
                     "Wrong cell field data");
  }

  void TestRegular3D() const
  {
    std::cout << "Testing threshold on 3D regular dataset" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    typedef vtkm::cont::CellSetPermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                           CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DRegularDataSet0();

    CellSetType cellset = dataset.GetCellSet(0).CastTo(CellSetType());

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet = threshold.Run(cellset,
                                              dataset.GetField("pointvar"),
                                              HasValue(20.1f),
                                              DeviceAdapter());
    vtkm::cont::Field cellField =
        threshold.ProcessCellField(dataset.GetField("cellvar"));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 2, "Wrong number of cells");

    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CastToArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                     cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;
    typedef vtkm::cont::CellSetPermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                           CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet0();

    CellSetType cellset = dataset.GetCellSet(0).CastTo(CellSetType());

    vtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet = threshold.Run(cellset,
                                              dataset.GetField("cellvar"),
                                              HasValue(100.1f),
                                              DeviceAdapter());
    vtkm::cont::Field cellField =
        threshold.ProcessCellField(dataset.GetField("cellvar"));

    VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CastToArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 100.1f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestRegular2D();
    this->TestRegular3D();
    this->TestExplicit3D();
  }
};

}

int UnitTestThreshold(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
    TestingThreshold<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
