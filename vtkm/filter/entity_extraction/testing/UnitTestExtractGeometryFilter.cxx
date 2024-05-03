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
#include <vtkm/filter/entity_extraction/ExtractGeometry.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingExtractGeometry
{
public:
  static void TestUniformByBox0()
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    vtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    vtkm::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::entity_extraction::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(true);
    extractGeometry.SetExtractBoundaryCells(false);
    extractGeometry.SetExtractOnlyBoundaryCells(false);

    vtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8), "Wrong result for ExtractGeometry");

    vtkm::filter::clean_grid::CleanGrid cleanGrid;
    cleanGrid.SetCompactPointFields(true);
    cleanGrid.SetMergePoints(false);
    vtkm::cont::DataSet cleanOutput = cleanGrid.Execute(output);

    vtkm::cont::ArrayHandle<vtkm::Float32> outCellData;
    cleanOutput.GetField("cellvar").GetData().AsArrayHandle(outCellData);

    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(0) == 21.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(7) == 42.f, "Wrong cell field data");

    vtkm::cont::ArrayHandle<vtkm::Float32> outPointData;
    cleanOutput.GetField("pointvar").GetData().AsArrayHandle(outPointData);
    VTKM_TEST_ASSERT(outPointData.ReadPortal().Get(0) == 99);
    VTKM_TEST_ASSERT(outPointData.ReadPortal().Get(7) == 90);
  }

  static void TestUniformByBox1()
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    vtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    vtkm::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::entity_extraction::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(false);
    extractGeometry.SetExtractBoundaryCells(false);
    extractGeometry.SetExtractOnlyBoundaryCells(false);

    vtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 56), "Wrong result for ExtractGeometry");

    vtkm::cont::ArrayHandle<vtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().AsArrayHandle(outCellData);

    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(0) == 0.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(55) == 63.f, "Wrong cell field data");
  }

  static void TestUniformByBox2()
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(0.5f, 0.5f, 0.5f);
    vtkm::Vec3f maxPoint(3.5f, 3.5f, 3.5f);
    vtkm::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::entity_extraction::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(true);
    extractGeometry.SetExtractBoundaryCells(true);
    extractGeometry.SetExtractOnlyBoundaryCells(false);

    vtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 64), "Wrong result for ExtractGeometry");

    vtkm::cont::ArrayHandle<vtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().AsArrayHandle(outCellData);

    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(0) == 0.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(63) == 63.f, "Wrong cell field data");
  }
  static void TestUniformByBox3()
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(0.5f, 0.5f, 0.5f);
    vtkm::Vec3f maxPoint(3.5f, 3.5f, 3.5f);
    vtkm::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    vtkm::filter::entity_extraction::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(true);
    extractGeometry.SetExtractBoundaryCells(true);
    extractGeometry.SetExtractOnlyBoundaryCells(true);

    vtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 56), "Wrong result for ExtractGeometry");

    vtkm::cont::ArrayHandle<vtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().AsArrayHandle(outCellData);

    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(0) == 0.f, "Wrong cell field data");
    VTKM_TEST_ASSERT(outCellData.ReadPortal().Get(55) == 63.f, "Wrong cell field data");
  }

  void operator()() const
  {
    TestingExtractGeometry::TestUniformByBox0();
    TestingExtractGeometry::TestUniformByBox1();
    TestingExtractGeometry::TestUniformByBox2();
    TestingExtractGeometry::TestUniformByBox3();
  }
};
}

int UnitTestExtractGeometryFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingExtractGeometry(), argc, argv);
}
