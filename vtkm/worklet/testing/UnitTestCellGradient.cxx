//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/Gradient.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestCellGradientUniform2D()
{
  std::cout << "Testing CellGradient Worklet on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> input;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> result;

  dataSet.GetField("pointvar").GetData().CopyTo(input);

  vtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);

  vtkm::Vec3f_32 expected[2] = { { 10, 30, 0 }, { 10, 30, 0 } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient worklet on 2D uniform data");
  }
}

void TestCellGradientUniform3D()
{
  std::cout << "Testing CellGradient Worklet on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> input;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> result;

  dataSet.GetField("pointvar").GetData().CopyTo(input);

  vtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);

  vtkm::Vec3f_32 expected[4] = {
    { 10.025f, 30.075f, 60.125f },
    { 10.025f, 30.075f, 60.125f },
    { 10.025f, 30.075f, 60.175f },
    { 10.025f, 30.075f, 60.175f },
  };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient worklet on 3D uniform data");
  }
}

void TestCellGradientUniform3DWithVectorField()
{
  std::cout
    << "Testing CellGradient and QCriterion Worklet with a vector field on 3D structured data"
    << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  vtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<vtkm::Vec3f_64> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> input = vtkm::cont::make_ArrayHandle(vec);

  //we need to add Vec3 array to the dataset
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_64, 3>> result;

  vtkm::worklet::GradientOutputFields<vtkm::Vec3f_64> extraOutput;
  extraOutput.SetComputeDivergence(false);
  extraOutput.SetComputeVorticity(false);
  extraOutput.SetComputeQCriterion(true);

  vtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input, extraOutput);

  VTKM_TEST_ASSERT((extraOutput.Gradient.GetNumberOfValues() == 4),
                   "Gradient field should be generated");
  VTKM_TEST_ASSERT((extraOutput.Divergence.GetNumberOfValues() == 0),
                   "Divergence field shouldn't be generated");
  VTKM_TEST_ASSERT((extraOutput.Vorticity.GetNumberOfValues() == 0),
                   "Vorticity field shouldn't be generated");
  VTKM_TEST_ASSERT((extraOutput.QCriterion.GetNumberOfValues() == 4),
                   "QCriterion field should be generated");

  vtkm::Vec<vtkm::Vec3f_64, 3> expected[4] = {
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec3f_64, 3> e = expected[i];
    vtkm::Vec<vtkm::Vec3f_64, 3> r = result.GetPortalConstControl().Get(i);

    VTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field CellGradient worklet on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field CellGradient worklet on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field CellGradient worklet on 3D uniform data");

    const vtkm::Vec3f_64 v(e[1][2] - e[2][1], e[2][0] - e[0][2], e[0][1] - e[1][0]);
    const vtkm::Vec3f_64 s(e[1][2] + e[2][1], e[2][0] + e[0][2], e[0][1] + e[1][0]);
    const vtkm::Vec3f_64 d(e[0][0], e[1][1], e[2][2]);

    //compute QCriterion
    vtkm::Float64 qcriterion =
      ((vtkm::Dot(v, v) / 2.0f) - (vtkm::Dot(d, d) + (vtkm::Dot(s, s) / 2.0f))) / 2.0f;

    vtkm::Float64 q = extraOutput.QCriterion.GetPortalConstControl().Get(i);

    std::cout << "qcriterion expected: " << qcriterion << std::endl;
    std::cout << "qcriterion actual: " << q << std::endl;

    VTKM_TEST_ASSERT(
      test_equal(qcriterion, q),
      "Wrong result for QCriterion field of CellGradient worklet on 3D uniform data");
  }
}

void TestCellGradientUniform3DWithVectorField2()
{
  std::cout << "Testing CellGradient Worklet with a vector field on 3D structured data" << std::endl
            << "Disabling Gradient computation and enabling Divergence, and Vorticity" << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  vtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<vtkm::Vec3f_64> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> input = vtkm::cont::make_ArrayHandle(vec);

  vtkm::worklet::GradientOutputFields<vtkm::Vec3f_64> extraOutput;
  extraOutput.SetComputeGradient(false);
  extraOutput.SetComputeDivergence(true);
  extraOutput.SetComputeVorticity(true);
  extraOutput.SetComputeQCriterion(false);

  vtkm::worklet::CellGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input, extraOutput);

  //Verify that the result is 0 size
  VTKM_TEST_ASSERT((result.GetNumberOfValues() == 0), "Gradient field shouldn't be generated");
  //Verify that the extra arrays are the correct size
  VTKM_TEST_ASSERT((extraOutput.Gradient.GetNumberOfValues() == 0),
                   "Gradient field shouldn't be generated");
  VTKM_TEST_ASSERT((extraOutput.Divergence.GetNumberOfValues() == 4),
                   "Divergence field should be generated");
  VTKM_TEST_ASSERT((extraOutput.Vorticity.GetNumberOfValues() == 4),
                   "Vorticity field should be generated");
  VTKM_TEST_ASSERT((extraOutput.QCriterion.GetNumberOfValues() == 0),
                   "QCriterion field shouldn't be generated");

  //Verify the contents of the other arrays
  vtkm::Vec<vtkm::Vec3f_64, 3> expected_gradients[4] = {
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } }
  };

  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec3f_64, 3> eg = expected_gradients[i];

    vtkm::Float64 d = extraOutput.Divergence.GetPortalConstControl().Get(i);
    VTKM_TEST_ASSERT(test_equal((eg[0][0] + eg[1][1] + eg[2][2]), d),
                     "Wrong result for Divergence on 3D uniform data");

    vtkm::Vec3f_64 ev(eg[1][2] - eg[2][1], eg[2][0] - eg[0][2], eg[0][1] - eg[1][0]);
    vtkm::Vec3f_64 v = extraOutput.Vorticity.GetPortalConstControl().Get(i);
    VTKM_TEST_ASSERT(test_equal(ev, v), "Wrong result for Vorticity on 3D uniform data");
  }
}

void TestCellGradientExplicit()
{
  std::cout << "Testing CellGradient Worklet on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> input;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> result;
  dataSet.GetField("pointvar").GetData().CopyTo(input);

  vtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);

  vtkm::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, -0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient worklet on 3D explicit data");
  }
}

void TestCellGradient()
{
  TestCellGradientUniform2D();
  TestCellGradientUniform3D();
  TestCellGradientUniform3DWithVectorField();
  TestCellGradientUniform3DWithVectorField2();
  TestCellGradientExplicit();
}
}

int UnitTestCellGradient(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellGradient, argc, argv);
}
