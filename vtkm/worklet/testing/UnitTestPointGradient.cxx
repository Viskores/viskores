//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/Gradient.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestPointGradientUniform2D()
{
  std::cout << "Testing PointGradient Worklet on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray);

  vtkm::Vec3f_32 expected[2] = { { 10, 30, 0 }, { 10, 30, 0 } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 2D uniform data",
                     "\nExpected ",
                     expected[i],
                     "\nGot ",
                     result.GetPortalConstControl().Get(i),
                     "\n");
  }
}

void TestPointGradientUniform3D()
{
  std::cout << "Testing PointGradient Worklet on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray);

  vtkm::Vec3f_32 expected[4] = {
    { 10.0f, 30.f, 60.1f },
    { 10.0f, 30.1f, 60.1f },
    { 10.0f, 30.1f, 60.2f },
    { 10.1f, 30.f, 60.2f },
  };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 3D uniform data",
                     "\nExpected ",
                     expected[i],
                     "\nGot ",
                     result.GetPortalConstControl().Get(i),
                     "\n");
  }
}

void TestPointGradientUniform3DWithVectorField()
{
  std::cout << "Testing PointGradient Worklet with a vector field on 3D structured data"
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

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);


  vtkm::Vec<vtkm::Vec3f_64, 3> expected[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec3f_64, 3> e = expected[i];
    vtkm::Vec<vtkm::Vec3f_64, 3> r = result.GetPortalConstControl().Get(i);

    VTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field PointGradient worklet on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field PointGradient worklet on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field PointGradient worklet on 3D uniform data");
  }
}

void TestPointGradientUniform3DWithVectorField2()
{
  std::cout << "Testing PointGradient Worklet with a vector field on 3D structured data"
            << std::endl
            << "Disabling Gradient computation and enabling Divergence, Vorticity, and QCriterion"
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

  vtkm::worklet::GradientOutputFields<vtkm::Vec3f_64> extraOutput;
  extraOutput.SetComputeGradient(false);
  extraOutput.SetComputeDivergence(true);
  extraOutput.SetComputeVorticity(true);
  extraOutput.SetComputeQCriterion(true);

  vtkm::worklet::PointGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input, extraOutput);

  //Verify that the result is 0 size
  VTKM_TEST_ASSERT((result.GetNumberOfValues() == 0), "Gradient field shouldn't be generated");
  //Verify that the extra arrays are the correct size
  VTKM_TEST_ASSERT((extraOutput.Gradient.GetNumberOfValues() == 0),
                   "Gradient field shouldn't be generated");
  VTKM_TEST_ASSERT((extraOutput.Divergence.GetNumberOfValues() == nVerts),
                   "Divergence field should be generated");
  VTKM_TEST_ASSERT((extraOutput.Vorticity.GetNumberOfValues() == nVerts),
                   "Vorticity field should be generated");
  VTKM_TEST_ASSERT((extraOutput.QCriterion.GetNumberOfValues() == nVerts),
                   "QCriterion field should be generated");

  vtkm::Vec<vtkm::Vec3f_64, 3> expected_gradients[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
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

    const vtkm::Vec3f_64 es(eg[1][2] + eg[2][1], eg[2][0] + eg[0][2], eg[0][1] + eg[1][0]);
    const vtkm::Vec3f_64 ed(eg[0][0], eg[1][1], eg[2][2]);

    //compute QCriterion
    vtkm::Float64 qcriterion =
      ((vtkm::Dot(ev, ev) / 2.0f) - (vtkm::Dot(ed, ed) + (vtkm::Dot(es, es) / 2.0f))) / 2.0f;

    vtkm::Float64 q = extraOutput.QCriterion.GetPortalConstControl().Get(i);

    VTKM_TEST_ASSERT(
      test_equal(qcriterion, q),
      "Wrong result for QCriterion field of PointGradient worklet on 3D uniform data");
  }
}

void TestPointGradientExplicit3D()
{
  std::cout << "Testing PointGradient Worklet on Explicit 3D data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet5();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray);

  //vtkm::cont::printSummary_ArrayHandle(result, std::cout, true);
  const int nVerts = 11;
  vtkm::Vec3f_32 expected[nVerts] = {
    { 10.0f, 40.2f, 30.1f },  { 27.4f, 40.1f, 10.1f },        { 17.425f, 40.0f, 10.1f },
    { -10.0f, 40.1f, 30.1f }, { 9.9f, -0.0500011f, 30.0f },   { 16.2125f, -4.55f, 10.0f },
    { 6.2f, -4.6f, 10.0f },   { -10.1f, -0.0999985f, 30.0f }, { 22.5125f, -4.575f, 10.025f },
    { 1.0f, -40.3f, 30.0f },  { 0.6f, -49.2f, 10.0f }
  };
  for (int i = 0; i < nVerts; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 3D explicit data",
                     "\nExpected ",
                     expected[i],
                     "\nGot ",
                     result.GetPortalConstControl().Get(i),
                     "\n");
  }
}

void TestPointGradientExplicit2D()
{
  std::cout << "Testing PointGradient Worklet on Explicit 2D data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DExplicitDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray);

  //vtkm::cont::printSummary_ArrayHandle(result, std::cout, true);
  const int nVerts = 16;
  vtkm::Vec3f_32 expected[nVerts] = {
    { -22.0f, -7.0f, 0.0f },       { -25.5f, -7.0f, 0.0f },         { -30.5f, 7.0f, 0.0f },
    { -32.0f, 16.0f, 0.0f },       { -23.0f, -42.0f, 0.0f },        { -23.25f, -17.0f, 0.0f },
    { -20.6667f, 1.33333f, 0.0f }, { -23.0f, 14.0f, 0.0f },         { -8.0f, -42.0f, 0.0f },
    { 2.91546f, -24.8357f, 0.0f }, { -0.140736f, -7.71853f, 0.0f }, { -5.0f, 12.0f, 0.0f },
    { 31.8803f, 1.0f, 0.0f },      { -44.8148f, 20.5f, 0.0f },      { 38.5653f, 5.86938f, 0.0f },
    { 26.3967f, 86.7934f, 0.0f }
  };

  for (int i = 0; i < nVerts; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 2D explicit data",
                     "\nExpected ",
                     expected[i],
                     "\nGot ",
                     result.GetPortalConstControl().Get(i),
                     "\n");
  }
}

void TestPointGradient()
{
  TestPointGradientUniform2D();
  TestPointGradientUniform3D();
  TestPointGradientUniform3DWithVectorField();
  TestPointGradientUniform3DWithVectorField2();
  TestPointGradientExplicit2D();
  TestPointGradientExplicit3D();
}
}

int UnitTestPointGradient(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointGradient, argc, argv);
}
