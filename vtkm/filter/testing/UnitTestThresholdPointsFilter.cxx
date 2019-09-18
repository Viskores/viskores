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

#include <vtkm/filter/ThresholdPoints.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingThresholdPoints
{
public:
  void TestRegular2D() const
  {
    std::cout << "Testing threshold points on 2D regular dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    vtkm::filter::ThresholdPoints thresholdPoints;
    thresholdPoints.SetThresholdBetween(40.0f, 71.0f);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);

    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 11), "Wrong result for ThresholdPoints");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 25),
                     "Wrong number of points for ThresholdPoints");

    vtkm::cont::Field pointField = output.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(12) == 50.0f,
                     "Wrong point field data");
  }

  void TestRegular3D() const
  {
    std::cout << "Testing threshold points on 3D regular dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    vtkm::filter::ThresholdPoints thresholdPoints;
    thresholdPoints.SetThresholdAbove(1.0f);
    thresholdPoints.SetCompactPoints(true);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);

    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27), "Wrong result for ThresholdPoints");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 27),
                     "Wrong number of points for ThresholdPoints");

    vtkm::cont::Field pointField = output.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(0) == 99.0f,
                     "Wrong point field data");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold points on 3D explicit dataset" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    vtkm::filter::ThresholdPoints thresholdPoints;
    thresholdPoints.SetThresholdBelow(50.0);
    thresholdPoints.SetCompactPoints(true);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);

    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 6), "Wrong result for ThresholdPoints");
    VTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 6),
                     "Wrong number of points for ThresholdPoints");

    vtkm::cont::Field pointField = output.GetField("pointvar");
    vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    VTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(4) == 10.f,
                     "Wrong point field data");
  }

  void TestExplicit3DZeroResults() const
  {
    std::cout << "Testing threshold on 3D explicit dataset with empty results" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    vtkm::filter::ThresholdPoints thresholdPoints;

    thresholdPoints.SetThresholdBetween(500.0, 600.0);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);
    VTKM_TEST_ASSERT(output.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 0), "Wrong result for ThresholdPoints");
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

int UnitTestThresholdPointsFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingThresholdPoints(), argc, argv);
}
