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

  vtkm::Vec<vtkm::Float32, 3> expected[2] = { { 10, 30, 0 }, { 10, 30, 0 } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 2D uniform data");
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

  vtkm::Vec<vtkm::Float32, 3> expected[4] = {
    { 10.0f, 30.f, 60.1f },
    { 10.0f, 30.1f, 60.1f },
    { 10.0f, 30.1f, 60.2f },
    { 10.1f, 30.f, 60.2f },
  };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 3D uniform data");
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
  std::vector<vtkm::Vec<vtkm::Float64, 3>> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> input = vtkm::cont::make_ArrayHandle(vec);

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);


  vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> expected[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> e = expected[i];
    vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> r = result.GetPortalConstControl().Get(i);

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
  std::vector<vtkm::Vec<vtkm::Float64, 3>> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> input = vtkm::cont::make_ArrayHandle(vec);

  vtkm::worklet::GradientOutputFields<vtkm::Vec<vtkm::Float64, 3>> extraOutput;
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

  vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> expected_gradients[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> eg = expected_gradients[i];

    vtkm::Float64 d = extraOutput.Divergence.GetPortalConstControl().Get(i);

    VTKM_TEST_ASSERT(test_equal((eg[0][0] + eg[1][1] + eg[2][2]), d),
                     "Wrong result for Divergence on 3D uniform data");

    vtkm::Vec<vtkm::Float64, 3> ev(eg[1][2] - eg[2][1], eg[2][0] - eg[0][2], eg[0][1] - eg[1][0]);
    vtkm::Vec<vtkm::Float64, 3> v = extraOutput.Vorticity.GetPortalConstControl().Get(i);
    VTKM_TEST_ASSERT(test_equal(ev, v), "Wrong result for Vorticity on 3D uniform data");

    const vtkm::Vec<vtkm::Float64, 3> es(
      eg[1][2] + eg[2][1], eg[2][0] + eg[0][2], eg[0][1] + eg[1][0]);
    const vtkm::Vec<vtkm::Float64, 3> ed(eg[0][0], eg[1][1], eg[2][2]);

    //compute QCriterion
    vtkm::Float64 qcriterion =
      ((vtkm::Dot(ev, ev) / 2.0f) - (vtkm::Dot(ed, ed) + (vtkm::Dot(es, es) / 2.0f))) / 2.0f;

    vtkm::Float64 q = extraOutput.QCriterion.GetPortalConstControl().Get(i);

    VTKM_TEST_ASSERT(
      test_equal(qcriterion, q),
      "Wrong result for QCriterion field of PointGradient worklet on 3D uniform data");
  }
}

void TestPointGradientExplicit()
{
  std::cout << "Testing PointGradient Worklet on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray);

  vtkm::Vec<vtkm::Float32, 3> expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, 0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 3D explicit data");
  }
}

void TestPointGradient()
{
  TestPointGradientUniform2D();
  TestPointGradientUniform3D();
  TestPointGradientUniform3DWithVectorField();
  TestPointGradientUniform3DWithVectorField2();
  TestPointGradientExplicit();
}
}

int UnitTestPointGradient(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointGradient, argc, argv);
}
