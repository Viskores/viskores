//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderUniform.h>
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
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 2,
                       "Wrong number of fields in the output dataset");

      vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
      output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

      VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                         cellFieldArray.ReadPortal().Get(0) == 100.1f,
                       "Wrong cell field data");
    }
    else
    {
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 2,
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
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 2,
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
      VTKM_TEST_ASSERT(output.GetNumberOfFields() == 2,
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

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 2,
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

    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 2,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 0, "field should be empty");

    // Make sure that the resulting data set can be successfully passed to another
    // simple filter using the cell set.
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.Execute(output);
  }

  static void TestAllOptions()
  {
    std::cout << "Testing combinations of all the supported options" << std::endl;

    auto input = vtkm::cont::DataSetBuilderUniform::Create(vtkm::Id2{ 4, 2 });
    static const vtkm::Vec2f pointvar[8] = { { 0.0f, 1.0f },   { 4.0f, 5.0f },  { 8.0f, 9.0f },
                                             { 12.0f, 13.0f }, { 2.0f, 3.0f },  { 6.0f, 7.0f },
                                             { 10.0f, 11.0f }, { 14.0f, 15.0f } };
    static const vtkm::Vec2f cellvar[3] = { { 0.0f, 2.0f }, { 3.0f, 6.0f }, { 7.0f, 14.0f } };
    input.AddPointField("pointvar", pointvar, 8);
    input.AddCellField("cellvar", cellvar, 3);

    vtkm::Id expected[] = { 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 1, 2, 3, 0, 3, 0, 2, 1,
                            1, 2, 1, 2, 1, 2, 0, 3, 0, 3, 0, 3, 0, 3, 0, 3, 0, 3,
                            2, 1, 3, 0, 2, 1, 1, 2, 1, 2, 0, 3, 0, 3, 1, 2, 0, 3 };

    vtkm::Float64 lower = 7.0;
    vtkm::Float64 upper = 11.0;

    vtkm::filter::entity_extraction::Threshold threshold;

    int failures = 0;
    for (int fieldType = 0, counter = 0; fieldType < 3; ++fieldType)
    {
      std::vector<std::string> combo;
      switch (fieldType)
      {
        case 0:
          threshold.SetActiveField("pointvar");
          threshold.SetAllInRange(false);
          combo.push_back("pointvar, any");
          break;
        case 1:
          threshold.SetActiveField("pointvar");
          threshold.SetAllInRange(true);
          combo.push_back("pointvar, all");
          break;
        case 2:
        default:
          threshold.SetActiveField("cellvar");
          combo.push_back("cellvar");
          break;
      }

      for (int thresholdFunction = 0; thresholdFunction < 3; ++thresholdFunction)
      {
        switch (thresholdFunction)
        {
          case 0:
            threshold.SetThresholdBelow(lower);
            combo.push_back("below");
            break;
          case 1:
            threshold.SetThresholdAbove(upper);
            combo.push_back("above");
            break;
          case 2:
          default:
            threshold.SetThresholdBetween(lower, upper);
            combo.push_back("between");
            break;
        }

        for (int componentMode = 0; componentMode < 3; ++componentMode)
        {
          switch (componentMode)
          {
            case 0:
              threshold.SetComponentToTest(1);
              combo.push_back("1st component");
              break;
            case 1:
              threshold.SetComponentToTestToAny();
              combo.push_back("any component");
              break;
            case 2:
            default:
              threshold.SetComponentToTestToAll();
              combo.push_back("all components");
              break;
          }

          for (int invert = 0; invert <= 1; ++invert, ++counter)
          {
            threshold.SetInvert(invert ? true : false);

            std::cout << counter << ". combo: ";
            for (const auto& str : combo)
            {
              std::cout << str << ", ";
            }
            std::cout << (invert ? "invert on" : "invert off");

            auto output = threshold.Execute(input);
            auto numOutputCells = output.GetNumberOfCells();
            if (numOutputCells == expected[counter])
            {
              std::cout << " ...Passed" << std::endl;
            }
            else
            {
              std::cout << "\nFAILED: expected " << expected[counter] << ", got " << numOutputCells
                        << std::endl;
              ++failures;
            }
          }

          combo.pop_back();
        }
        combo.pop_back();
      }
      combo.pop_back();
    }

    VTKM_TEST_ASSERT(failures == 0, "Some combinations have failed");
  }

  void operator()() const
  {
    TestingThreshold::TestRegular2D(false);
    TestingThreshold::TestRegular2D(true);
    TestingThreshold::TestRegular3D(false);
    TestingThreshold::TestRegular3D(true);
    TestingThreshold::TestExplicit3D();
    TestingThreshold::TestExplicit3DZeroResults();
    TestingThreshold::TestAllOptions();
  }
};
}

int UnitTestThresholdFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThreshold(), argc, argv);
}
