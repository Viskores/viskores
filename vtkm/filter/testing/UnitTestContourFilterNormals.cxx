//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CleanGrid.h>

#include <vtkm/filter/Contour.h>

#include <vtkm/io/VTKDataSetWriter.h>

namespace vtkm_ut_mc_normals
{

vtkm::cont::DataSet MakeNormalsTestDataSet()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id3 dimensions(3, 4, 4);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  const int nVerts = 48;
  vtkm::Float32 vars[nVerts] = { 60.764f,  107.555f, 80.524f,  63.639f,  131.087f, 83.4f,
                                 98.161f,  165.608f, 117.921f, 37.353f,  84.145f,  57.114f,
                                 95.202f,  162.649f, 114.962f, 115.896f, 215.56f,  135.657f,
                                 150.418f, 250.081f, 170.178f, 71.791f,  139.239f, 91.552f,
                                 95.202f,  162.649f, 114.962f, 115.896f, 215.56f,  135.657f,
                                 150.418f, 250.081f, 170.178f, 71.791f,  139.239f, 91.552f,
                                 60.764f,  107.555f, 80.524f,  63.639f,  131.087f, 83.4f,
                                 98.161f,  165.608f, 117.921f, 37.353f,  84.145f,  57.114f };

  //Set point and cell scalar
  dataSet.AddPointField("pointvar", vars, nVerts);

  return dataSet;
}

void TestNormals(const vtkm::cont::DataSet& dataset, bool structured)
{
  const vtkm::Id numVerts = 16;

  //Calculated using PointGradient
  const vtkm::Vec3f hq_ug[numVerts] = {
    { 0.1510f, 0.6268f, 0.7644f },   { 0.1333f, -0.3974f, 0.9079f },
    { 0.1626f, 0.7642f, 0.6242f },   { 0.3853f, 0.6643f, 0.6405f },
    { -0.1337f, 0.7136f, 0.6876f },  { 0.7705f, -0.4212f, 0.4784f },
    { -0.7360f, -0.4452f, 0.5099f }, { 0.1234f, -0.8871f, 0.4448f },
    { 0.1626f, 0.7642f, -0.6242f },  { 0.3853f, 0.6643f, -0.6405f },
    { -0.1337f, 0.7136f, -0.6876f }, { 0.1510f, 0.6268f, -0.7644f },
    { 0.7705f, -0.4212f, -0.4784f }, { -0.7360f, -0.4452f, -0.5099f },
    { 0.1234f, -0.8871f, -0.4448f }, { 0.1333f, -0.3974f, -0.9079f }
  };

  //Calculated using StructuredPointGradient
  const vtkm::Vec3f hq_sg[numVerts] = {
    { 0.151008f, 0.626778f, 0.764425f },   { 0.133328f, -0.397444f, 0.907889f },
    { 0.162649f, 0.764163f, 0.624180f },   { 0.385327f, 0.664323f, 0.640467f },
    { -0.133720f, 0.713645f, 0.687626f },  { 0.770536f, -0.421248f, 0.478356f },
    { -0.736036f, -0.445244f, 0.509910f }, { 0.123446f, -0.887088f, 0.444788f },
    { 0.162649f, 0.764163f, -0.624180f },  { 0.385327f, 0.664323f, -0.640467f },
    { -0.133720f, 0.713645f, -0.687626f }, { 0.151008f, 0.626778f, -0.764425f },
    { 0.770536f, -0.421248f, -0.478356f }, { -0.736036f, -0.445244f, -0.509910f },
    { 0.123446f, -0.887088f, -0.444788f }, { 0.133328f, -0.397444f, -0.907889f }
  };
  //Calculated using FlyingEdges and Y axis iteration which causes
  //the points to be in a different order
  const vtkm::Id fe_y_alg_ordering[numVerts] = { 0, 1,  3,  5,  4, 6,  2,  7,
                                                 9, 12, 10, 13, 8, 14, 11, 15 };

  //Calculated using normals of the output triangles
  const vtkm::Vec3f fast[numVerts] = {
    { -0.1351f, 0.4377f, 0.8889f },  { 0.2863f, -0.1721f, 0.9426f },
    { 0.3629f, 0.8155f, 0.4509f },   { 0.8486f, 0.3560f, 0.3914f },
    { -0.8315f, 0.4727f, 0.2917f },  { 0.9395f, -0.2530f, 0.2311f },
    { -0.9105f, -0.0298f, 0.4124f }, { -0.1078f, -0.9585f, 0.2637f },
    { -0.2538f, 0.8534f, -0.4553f }, { 0.8953f, 0.3902f, -0.2149f },
    { -0.8295f, 0.4188f, -0.3694f }, { 0.2434f, 0.4297f, -0.8695f },
    { 0.8951f, -0.1347f, -0.4251f }, { -0.8467f, -0.4258f, -0.3191f },
    { 0.2164f, -0.9401f, -0.2635f }, { -0.1589f, -0.1642f, -0.9735f }
  };

  //When using the Y axis algorithm the cells are generated in a different
  //order.
  const vtkm::Vec3f fast_fe_y[numVerts] = {
    { -0.243433f, -0.429741f, -0.869519f }, { 0.158904f, 0.164214f, -0.973542f },
    { -0.895292f, -0.390217f, -0.214903f }, { -0.895057f, 0.134692f, -0.425125f },
    { 0.829547f, -0.418793f, -0.36941f },   { 0.846705f, 0.425787f, -0.319054f },
    { 0.253811f, -0.853394f, -0.4553f },    { -0.216381f, 0.940084f, -0.263478f },
    { -0.848579f, -0.35602f, 0.391362f },   { -0.93948f, 0.252957f, 0.231065f },
    { 0.831549f, -0.472663f, 0.291744f },   { 0.910494f, 0.0298277f, 0.412446f },
    { -0.362862f, -0.815464f, 0.450944f },  { 0.107848f, 0.958544f, 0.263748f },
    { 0.135131f, -0.437674f, 0.888921f },   { -0.286251f, 0.172078f, 0.942576f }
  };

  vtkm::cont::ArrayHandle<vtkm::Vec3f> normals;

  vtkm::filter::Contour mc;
  mc.SetIsoValue(0, 200);
  mc.SetGenerateNormals(true);

  // Test default normals generation: high quality for structured, fast for unstructured.
  auto expected = structured ? hq_sg : fast;

  mc.SetActiveField("pointvar");
  auto result = mc.Execute(dataset);
  result.GetField("normals").GetData().AsArrayHandle(normals);
  VTKM_TEST_ASSERT(normals.GetNumberOfValues() == numVerts,
                   "Wrong number of values in normals field");

  //determine if we are using flying edge Y axis algorithm by checking the first normal value that differs
  const bool using_fe_y_alg_ordering =
    test_equal(normals.ReadPortal().Get(2), expected[fe_y_alg_ordering[2]], 0.001);
  {
    auto normalPotals = normals.ReadPortal();
    for (vtkm::Id i = 0; i < numVerts; ++i)
    {
      auto expected_v = !using_fe_y_alg_ordering ? expected[i] : expected[fe_y_alg_ordering[i]];
      VTKM_TEST_ASSERT(test_equal(normalPotals.Get(i), expected_v, 0.001),
                       "Result (",
                       normalPotals.Get(i),
                       ") does not match expected value (",
                       expected_v,
                       ") vert ",
                       i);
    }
  }

  // Test the other normals generation method
  if (structured)
  {
    mc.SetComputeFastNormalsForStructured(true);
    expected = fast;
    if (using_fe_y_alg_ordering)
    {
      expected = fast_fe_y;
    }
  }
  else
  {
    mc.SetComputeFastNormalsForUnstructured(false);
    expected = hq_ug;
  }

  result = mc.Execute(dataset);
  result.GetField("normals").GetData().AsArrayHandle(normals);
  VTKM_TEST_ASSERT(normals.GetNumberOfValues() == numVerts,
                   "Wrong number of values in normals field");

  {
    auto normalPotals = normals.ReadPortal();
    for (vtkm::Id i = 0; i < numVerts; ++i)
    {
      bool equal = test_equal(normalPotals.Get(i), expected[i], 0.001);
      VTKM_TEST_ASSERT(equal,
                       "Result (",
                       normalPotals.Get(i),
                       ") does not match expected value (",
                       expected[i],
                       ") vert ",
                       i);
    }
  }
}

void TestContourNormals()
{
  std::cout << "Testing Contour normals generation" << std::endl;

  std::cout << "\tStructured dataset\n";
  vtkm::cont::DataSet dataset = MakeNormalsTestDataSet();
  TestNormals(dataset, true);

  std::cout << "\tUnstructured dataset\n";
  vtkm::filter::CleanGrid makeUnstructured;
  makeUnstructured.SetCompactPointFields(false);
  makeUnstructured.SetMergePoints(false);
  makeUnstructured.SetFieldsToPass("pointvar");
  auto result = makeUnstructured.Execute(dataset);
  TestNormals(result, false);
}

} // namespace

int UnitTestContourFilterNormals(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(vtkm_ut_mc_normals::TestContourNormals, argc, argv);
}
