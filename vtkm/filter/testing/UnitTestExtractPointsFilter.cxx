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

#include <vtkm/filter/ExtractPoints.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingExtractPoints
{
public:
  void TestUniformByBox0() const
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    vtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    auto box = vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(true);
    extractPoints.SetCompactPoints(true);

    vtkm::cont::DataSet output = extractPoints.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27), "Wrong result for ExtractPoints");

    vtkm::cont::ArrayHandle<vtkm::Float32> outPointData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);

    VTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractPoints filter");

    VTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 99.0f,
                     "Wrong point field data");
    VTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(26) == 97.0f,
                     "Wrong point field data");
  }

  void TestUniformByBox1() const
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    vtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    auto box = vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(false);
    extractPoints.SetCompactPoints(true);

    vtkm::cont::DataSet output = extractPoints.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 98), "Wrong result for ExtractPoints");

    vtkm::cont::ArrayHandle<vtkm::Float32> outPointData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);

    VTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractPoints filter");

    for (vtkm::Id i = 0; i < output.GetCellSet().GetNumberOfPoints(); i++)
    {
      VTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(i) == 0.0f,
                       "Wrong point field data");
    }
  }

  void TestUniformBySphere() const
  {
    std::cout << "Testing extract points with implicit function (sphere):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f center(2.f, 2.f, 2.f);
    vtkm::FloatDefault radius(1.8f);
    auto sphere = vtkm::cont::make_ImplicitFunctionHandle<vtkm::Sphere>(center, radius);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(sphere);
    extractPoints.SetExtractInside(true);

    vtkm::cont::DataSet output = extractPoints.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27), "Wrong result for ExtractPoints");
  }

  void TestExplicitByBox0() const
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Implicit function
    vtkm::Vec3f minPoint(0.f, 0.f, 0.f);
    vtkm::Vec3f maxPoint(1.f, 1.f, 1.f);
    auto box = vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(true);

    vtkm::cont::DataSet output = extractPoints.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8), "Wrong result for ExtractPoints");
  }

  void TestExplicitByBox1() const
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Implicit function
    vtkm::Vec3f minPoint(0.f, 0.f, 0.f);
    vtkm::Vec3f maxPoint(1.f, 1.f, 1.f);
    auto box = vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(false);

    vtkm::cont::DataSet output = extractPoints.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 3), "Wrong result for ExtractPoints");
  }

  void operator()() const
  {
    this->TestUniformByBox0();
    this->TestUniformByBox1();
    this->TestUniformBySphere();
    this->TestExplicitByBox0();
    this->TestExplicitByBox1();
  }
};
}

int UnitTestExtractPointsFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingExtractPoints(), argc, argv);
}
