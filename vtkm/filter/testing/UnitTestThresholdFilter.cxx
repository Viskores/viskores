//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/Threshold.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingThreshold
{
public:
  void TestRegular2D() const
  {
    std::cout << "Testing threshold on 2D regular dataset" << std::endl;

    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet0();

    vtkm::filter::Threshold threshold;
    threshold.SetLowerThreshold(60.1);
    threshold.SetUpperThreshold(60.1);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 200.1f,
                     "Wrong cell field data");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::CleanGrid clean;
    clean.Execute(output);
  }

  void TestRegular3D() const
  {
    std::cout << "Testing threshold on 3D regular dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    vtkm::filter::Threshold threshold;

    threshold.SetLowerThreshold(20.1);
    threshold.SetUpperThreshold(20.1);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::CleanGrid clean;
    clean.Execute(output);
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::Threshold threshold;

    threshold.SetLowerThreshold(20.1);
    threshold.SetUpperThreshold(20.1);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::CleanGrid clean;
    clean.Execute(output);
  }

  void TestExplicit3DZeroResults() const
  {
    std::cout << "Testing threshold on 3D explicit dataset with empty results" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::Threshold threshold;

    threshold.SetLowerThreshold(500.1);
    threshold.SetUpperThreshold(500.1);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 0, "field should be empty");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::CleanGrid clean;
    clean.Execute(output);
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

int UnitTestThresholdFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThreshold(), argc, argv);
}
