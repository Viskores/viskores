//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/vector_analysis/Gradient.h>

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestCellGradientUniform3D()
{
  std::cout << "Testing Gradient Filter with cell output on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::filter::vector_analysis::Gradient gradient;
  gradient.SetOutputFieldName("Gradient");

  gradient.SetComputeVorticity(true);  //this won't work as we have a scalar field
  gradient.SetComputeQCriterion(true); //this won't work as we have a scalar field

  gradient.SetActiveField("pointvar");

  vtkm::cont::DataSet result;

  // We provocate this exception
  try
  {
    result = gradient.Execute(dataSet);
    VTKM_TEST_FAIL("Gradient attempted to compute Vorticity or QCriterion with scalars");
  }
  catch (vtkm::cont::ErrorFilterExecution&)
  {
    // We should exit in this catch
  }
}

void TestCellGradientUniform3DWithVectorField()
{
  std::cout << "Testing Gradient Filter with vector cell output on 3D structured data" << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  vtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<vtkm::Vec3f_64> vec(nVerts);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> input =
    vtkm::cont::make_ArrayHandle(vec, vtkm::CopyFlag::On);
  dataSet.AddPointField("vec_pointvar", input);

  //we need to add Vec3 array to the dataset
  vtkm::filter::vector_analysis::Gradient gradient;
  gradient.SetOutputFieldName("vec_gradient");
  gradient.SetComputeDivergence(true);
  gradient.SetComputeVorticity(true);
  gradient.SetComputeQCriterion(true);
  gradient.SetActiveField("vec_pointvar");

  vtkm::cont::DataSet result = gradient.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasCellField("vec_gradient"), "Result field missing.");

  //verify that the vorticity and qcriterion fields DO exist
  VTKM_TEST_ASSERT(result.HasField("Divergence"));
  VTKM_TEST_ASSERT(result.HasField("Vorticity"));
  VTKM_TEST_ASSERT(result.HasField("QCriterion"));

  VTKM_TEST_ASSERT(test_equal_ArrayHandles(
    result.GetCellField("vec_gradient").GetData(),
    vtkm::cont::make_ArrayHandle<vtkm::Vec<vtkm::Vec3f_64, 3>>(
      { { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
        { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
        { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } },
        { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } } })));

  VTKM_TEST_ASSERT(test_equal_ArrayHandles(
    result.GetCellField("Divergence").GetData(),
    vtkm::cont::make_ArrayHandle<vtkm::Float64>({ 100.225, 100.225, 100.275, 100.275 })));

  VTKM_TEST_ASSERT(test_equal_ArrayHandles(
    result.GetCellField("Vorticity").GetData(),
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f_64>({ { -30.05, 50.1, -20.05 },
                                                   { -30.05, 50.1, -20.05 },
                                                   { -30.1, 50.15, -20.05 },
                                                   { -30.1, 50.15, -20.05 } })));

  VTKM_TEST_ASSERT(test_equal_ArrayHandles(
    result.GetCellField("QCriterion").GetData(),
    vtkm::cont::make_ArrayHandle<vtkm::Float64>({ -5022.53, -5022.53, -5027.54, -5027.54 })));
}


void TestPointGradientUniform3DWithVectorField()
{
  std::cout << "Testing Gradient Filter with vector point output on 3D structured data"
            << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  vtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<vtkm::Vec3f_64> vec(nVerts);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> input =
    vtkm::cont::make_ArrayHandle(vec, vtkm::CopyFlag::On);
  dataSet.AddPointField("vec_pointvar", input);

  //we need to add Vec3 array to the dataset
  vtkm::filter::vector_analysis::Gradient gradient;
  gradient.SetComputePointGradient(true);
  gradient.SetOutputFieldName("vec_gradient");
  gradient.SetActiveField("vec_pointvar");
  vtkm::cont::DataSet result = gradient.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasPointField("vec_gradient"), "Result field missing.");

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_64, 3>> resultArrayHandle;
  result.GetPointField("vec_gradient").GetData().AsArrayHandle(resultArrayHandle);
  vtkm::Vec<vtkm::Vec3f_64, 3> expected[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec3f_64, 3> e = expected[i];
    vtkm::Vec<vtkm::Vec3f_64, 3> r = resultArrayHandle.ReadPortal().Get(i);

    VTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
  }
}



void TestGradient()
{
  TestCellGradientUniform3D();
  TestCellGradientUniform3DWithVectorField();
  TestPointGradientUniform3DWithVectorField();
}
}

int UnitTestGradientUniform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGradient, argc, argv);
}
