//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/geometry_refinement/Shrink.h>

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{

const std::array<vtkm::FloatDefault, 7> expectedPointVar{
  { 10.1f, 20.1f, 30.2f, 30.2f, 20.1f, 40.2f, 50.3f }
};

const std::array<vtkm::Id, 7> expectedConnectivityArray{ { 0, 1, 2, 3, 4, 5, 6 } };

const vtkm::Vec3f expectedCoords[7]{ { 0.333333f, 0.166666f, 0.0f }, { 0.833333f, 0.166666f, 0.0f },
                                     { 0.833333f, 0.666666f, 0.0f }, { 1.25f, 1.0f, 0.0f },
                                     { 1.25f, 0.5f, 0.0f },          { 1.75f, 1.0f, 0.0f },
                                     { 1.75f, 1.5f, 0.0f } };

const vtkm::FloatDefault expectedPointValueCube1[8]{ 10.1f, 20.1f, 50.2f,  40.1f,
                                                     70.2f, 80.2f, 110.3f, 100.3f };
const vtkm::Vec3f expectedCoordsCell1[8]{ { 0.4f, 0.4f, 0.4f }, { 0.6f, 0.4f, 0.4f },
                                          { 0.6f, 0.6f, 0.4f }, { 0.4f, 0.6f, 0.4f },
                                          { 0.4f, 0.4f, 0.6f }, { 0.6f, 0.4f, 0.6f },
                                          { 0.6f, 0.6f, 0.6f }, { 0.4f, 0.6f, 0.6f } };


void TestWithExplicitData()
{
  vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DExplicitDataSet0();

  vtkm::filter::geometry_refinement::Shrink shrink;
  shrink.SetFieldsToPass({ "pointvar", "cellvar" });

  VTKM_TEST_ASSERT(test_equal(shrink.GetShrinkFactor(), 0.5f), "Wrong shrink factor default value");

  // Test shrink factor clamping
  shrink.SetShrinkFactor(1.5f);
  VTKM_TEST_ASSERT(test_equal(shrink.GetShrinkFactor(), 1.0f), "Shrink factor not limited to 1");

  shrink.SetShrinkFactor(-0.5f);
  VTKM_TEST_ASSERT(test_equal(shrink.GetShrinkFactor(), 0.0f),
                   "Shrink factor is not always positive");

  shrink.SetShrinkFactor(0.5f);

  vtkm::cont::DataSet output = shrink.Execute(dataSet);
  VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), dataSet.GetNumberOfCells()),
                   "Wrong number of cells for Shrink filter");
  VTKM_TEST_ASSERT(test_equal(output.GetNumberOfPoints(), 7), "Wrong number of points for Shrink");


  vtkm::cont::ArrayHandle<vtkm::Float32> outCellData =
    output.GetField("cellvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  VTKM_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(0), 100.1f), "Wrong cell field data");
  VTKM_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(1), 100.2f), "Wrong cell field data");

  vtkm::cont::ArrayHandle<vtkm::Float32> outPointData =
    output.GetField("pointvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  for (vtkm::IdComponent i = 0; i < outPointData.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(outPointData.ReadPortal().Get(i), expectedPointVar[i]),
                     "Wrong point field data");
  }

  {
    const auto connectivityArray =
      output.GetCellSet().AsCellSet<vtkm::cont::CellSetExplicit<>>().GetConnectivityArray(
        vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto connectivityArrayPortal = connectivityArray.ReadPortal();

    for (vtkm::IdComponent i = 0; i < connectivityArray.GetNumberOfValues(); i++)
    {
      VTKM_TEST_ASSERT(test_equal(connectivityArrayPortal.Get(i), expectedConnectivityArray[i]),
                       "Wrong connectivity array value");
    }
  }

  auto newCoords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  auto newCoordsP = newCoords.ReadPortal();

  for (vtkm::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                     "Wrong point coordinates");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                     "Wrong point coordinates");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                     "Wrong point coordinates");
  }
}


void TestWithUniformData()
{
  vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet0();

  vtkm::filter::geometry_refinement::Shrink shrink;
  shrink.SetFieldsToPass({ "pointvar", "cellvar" });

  shrink.SetShrinkFactor(0.2f);

  vtkm::cont::DataSet output = shrink.Execute(dataSet);

  VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), dataSet.GetNumberOfCells()),
                   "Number of cells changed after filtering");
  VTKM_TEST_ASSERT(test_equal(output.GetNumberOfPoints(), 4 * 8), "Wrong number of points");

  vtkm::cont::ArrayHandle<vtkm::Float32> outCellData =
    output.GetField("cellvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  VTKM_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(0), 100.1), "Wrong cell field data");
  VTKM_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(1), 100.2), "Wrong cell field data");
  VTKM_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(2), 100.3), "Wrong cell field data");
  VTKM_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(3), 100.4), "Wrong cell field data");

  vtkm::cont::ArrayHandle<vtkm::Float32> outPointData =
    output.GetField("pointvar").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  for (vtkm::IdComponent i = 0; i < 8; i++) // Test for the first cell only
  {
    VTKM_TEST_ASSERT(test_equal(outPointData.ReadPortal().Get(i), expectedPointValueCube1[i]),
                     "Wrong cell field data");
  }

  auto newCoords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  auto newCoordsP = newCoords.ReadPortal();

  for (vtkm::IdComponent i = 0; i < 8; i++)
  {
    std::cout << newCoordsP.Get(i)[0] << " " << expectedCoordsCell1[i][0] << std::endl;
    std::cout << newCoordsP.Get(i)[1] << " " << expectedCoordsCell1[i][1] << std::endl;
    std::cout << newCoordsP.Get(i)[2] << " " << expectedCoordsCell1[i][2] << std::endl;

    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoordsCell1[i][0]),
                     "Wrong point coordinates");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoordsCell1[i][1]),
                     "Wrong point coordinates");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoordsCell1[i][2]),
                     "Wrong point coordinates");
  }
}


void TestShrinkFilter()
{
  TestWithExplicitData();
  TestWithUniformData();
}

} // anonymous namespace

int UnitTestShrinkFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestShrinkFilter, argc, argv);
}
