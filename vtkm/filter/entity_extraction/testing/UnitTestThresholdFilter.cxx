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
#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/entity_extraction/Threshold.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingThreshold
{
public:
  static void TestRegular2D(bool returnAllInRange)
  {
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet0();
    vtkm::filter::entity_extraction::Threshold threshold;

    if (returnAllInRange)
    {
      std::cout << "Testing threshold on 2D regular dataset returning values 'all in range'"
                << std::endl;
      threshold.SetLowerThreshold(10);
      threshold.SetUpperThreshold(60);
    }
    else
    {
      std::cout << "Testing threshold on 2D regular dataset returning values 'part in range'"
                << std::endl;
      threshold.SetLowerThreshold(60);
      threshold.SetUpperThreshold(61);
    }

    threshold.SetAllInRange(returnAllInRange);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    if (returnAllInRange)
    {
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                       "Wrong number of fields in the output dataset");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
      output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f,
                       "Wrong cell field data");
    }
    else
    {
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                       "Wrong number of fields in the output dataset");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
      output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                         cellFieldArray.ReadPortal().Get(0) == 200.1f,
                       "Wrong cell field data");
    }

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.Execute(output);
  }

  static void TestRegular3D(bool returnAllInRange)
  {
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();
    vtkm::filter::entity_extraction::Threshold threshold;

    if (returnAllInRange)
    {
      std::cout << "Testing threshold on 3D regular dataset returning values 'all in range'"
                << std::endl;
      threshold.SetLowerThreshold(10.1);
      threshold.SetUpperThreshold(180);
    }
    else
    {
      std::cout << "Testing threshold on 3D regular dataset returning values 'part in range'"
                << std::endl;
      threshold.SetLowerThreshold(20);
      threshold.SetUpperThreshold(21);
    }

    threshold.SetAllInRange(returnAllInRange);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    if (returnAllInRange)
    {
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                       "Wrong number of fields in the output dataset");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
      output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 3 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f &&
                         cellFieldArray.ReadPortal().Get(1) == 100.2f &&
                         cellFieldArray.ReadPortal().Get(2) == 100.3f,
                       "Wrong cell field data");
    }
    else
    {
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                       "Wrong number of fields in the output dataset");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
      output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f &&
                         cellFieldArray.ReadPortal().Get(1) == 100.2f,
                       "Wrong cell field data");
    }

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.Execute(output);
  }

  static void TestExplicit3D()
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::entity_extraction::Threshold threshold;

    threshold.SetLowerThreshold(20);
    threshold.SetUpperThreshold(21);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.ReadPortal().Get(0) == 100.1f &&
                       cellFieldArray.ReadPortal().Get(1) == 100.2f,
                     "Wrong cell field data");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.Execute(output);
  }

  static void TestExplicit3DZeroResults()
  {
    std::cout << "Testing threshold on 3D explicit dataset with empty results" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::entity_extraction::Threshold threshold;

    threshold.SetLowerThreshold(500);
    threshold.SetUpperThreshold(500.1);
    threshold.SetActiveField("pointvar");
    threshold.SetFieldsToPass("cellvar");
    auto output = threshold.Execute(dataset);

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 0, "field should be empty");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.Execute(output);
  }

  void operator()() const
  {
    TestingThreshold::TestRegular2D(false);
    TestingThreshold::TestRegular2D(true);
    TestingThreshold::TestRegular3D(false);
    TestingThreshold::TestRegular3D(true);
    TestingThreshold::TestExplicit3D();
    TestingThreshold::TestExplicit3DZeroResults();
  }
};
}

int UnitTestThresholdFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThreshold(), argc, argv);
}
