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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/Threshold.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace {


class TestingThreshold
{
public:
  void TestRegular2D() const
  {
    std::cout << "Testing threshold on 2D regular dataset" << std::endl;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet0();

    vtkm::filter::Threshold threshold;
    vtkm::filter::DataSetResult result;

    threshold.SetLowerThreshold(60.1);
    threshold.SetUpperThreshold(60.1);
    result = threshold.Execute(dataset, dataset.GetField("pointvar"));

    threshold.MapFieldOntoOutput(result, dataset.GetField("cellvar") );

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");

    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    OutCellFieldArrayHandleType cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 200.1f,
                     "Wrong cell field data");
  }

  void TestRegular3D() const
  {
    std::cout << "Testing threshold on 3D regular dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    vtkm::filter::Threshold threshold;
    vtkm::filter::DataSetResult result;

    threshold.SetLowerThreshold(20.1);
    threshold.SetUpperThreshold(20.1);
    result = threshold.Execute(dataset, std::string("pointvar"));

    threshold.MapFieldOntoOutput(result, dataset.GetField("cellvar") );

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");

    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    OutCellFieldArrayHandleType cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                     cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::Threshold threshold;
    vtkm::filter::DataSetResult result;

    threshold.SetLowerThreshold(20.1);
    threshold.SetUpperThreshold(20.1);
    result = threshold.Execute(dataset, std::string("pointvar"));

    threshold.MapFieldOntoOutput(result, dataset.GetField("cellvar") );

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");

    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    OutCellFieldArrayHandleType cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                     cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");
  }

  void TestExplicit3DZeroResults() const
  {
    std::cout << "Testing threshold on 3D explicit dataset with empty results" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::Threshold threshold;
    vtkm::filter::DataSetResult result;

    threshold.SetLowerThreshold(500.1);
    threshold.SetUpperThreshold(500.1);
    result = threshold.Execute(dataset, std::string("pointvar"));

    VTKM_TEST_ASSERT(result.IsValid(), "threshold algorithm should return true");

    threshold.MapFieldOntoOutput(result, dataset.GetField("cellvar") );

    vtkm::cont::DataSet output = result.GetDataSet();
    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");

    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    OutCellFieldArrayHandleType cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 0, "field should be empty");
  }

  void operator()() const
  {
    this->TestRegular2D();
    this->TestRegular3D();
    this->TestExplicit3D();
    this->TestExplicit3DZeroResults();
  }
};

}

int UnitTestThresholdFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestingThreshold());
}