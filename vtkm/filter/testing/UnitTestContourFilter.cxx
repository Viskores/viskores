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
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CleanGrid.h>

#include <vtkm/filter/Contour.h>
#include <vtkm/filter/Contour.hxx>
#include <vtkm/source/Tangle.h>

namespace vtkm_ut_mc_filter
{
class EuclideanNorm
{
public:
  VTKM_EXEC_CONT
  EuclideanNorm()
    : Reference(0., 0., 0.)
  {
  }
  VTKM_EXEC_CONT
  EuclideanNorm(vtkm::Vec3f_32 reference)
    : Reference(reference)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Float32 operator()(vtkm::Vec3f_32 v) const
  {
    vtkm::Vec3f_32 d(
      v[0] - this->Reference[0], v[1] - this->Reference[1], v[2] - this->Reference[2]);
    return vtkm::Magnitude(d);
  }

private:
  vtkm::Vec3f_32 Reference;
};

class CubeGridConnectivity
{
public:
  VTKM_EXEC_CONT
  CubeGridConnectivity()
    : Dimension(1)
    , DimSquared(1)
    , DimPlus1Squared(4)
  {
  }
  VTKM_EXEC_CONT
  CubeGridConnectivity(vtkm::Id dim)
    : Dimension(dim)
    , DimSquared(dim * dim)
    , DimPlus1Squared((dim + 1) * (dim + 1))
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id vertex) const
  {
    using HexTag = vtkm::CellShapeTagHexahedron;
    using HexTraits = vtkm::CellTraits<HexTag>;

    vtkm::Id cellId = vertex / HexTraits::NUM_POINTS;
    vtkm::Id localId = vertex % HexTraits::NUM_POINTS;
    vtkm::Id globalId =
      (cellId + cellId / this->Dimension + (this->Dimension + 1) * (cellId / (this->DimSquared)));

    switch (localId)
    {
      case 0:
        break;
      case 1:
        globalId += 1;
        break;
      case 2:
        globalId += this->Dimension + 2;
        break;
      case 3:
        globalId += this->Dimension + 1;
        break;
      case 4:
        globalId += this->DimPlus1Squared;
        break;
      case 5:
        globalId += this->DimPlus1Squared + 1;
        break;
      case 6:
        globalId += this->Dimension + this->DimPlus1Squared + 2;
        break;
      case 7:
        globalId += this->Dimension + this->DimPlus1Squared + 1;
        break;
    }

    return globalId;
  }

private:
  vtkm::Id Dimension;
  vtkm::Id DimSquared;
  vtkm::Id DimPlus1Squared;
};

class MakeRadiantDataSet
{
public:
  using CoordinateArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using DataArrayHandle =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleUniformPointCoordinates, EuclideanNorm>;
  using ConnectivityArrayHandle =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleCounting<vtkm::Id>,
                                     CubeGridConnectivity>;

  using CellSet = vtkm::cont::CellSetSingleType<
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleCounting<vtkm::Id>,
                                     CubeGridConnectivity>::StorageTag>;

  vtkm::cont::DataSet Make3DRadiantDataSet(vtkm::IdComponent dim = 5);
};

class PolicyRadiantDataSet : public vtkm::filter::PolicyBase<PolicyRadiantDataSet>
{
public:
  using TypeListRadiantCellSetTypes = vtkm::List<MakeRadiantDataSet::CellSet>;

  using AllCellSetList = TypeListRadiantCellSetTypes;
};

inline vtkm::cont::DataSet MakeRadiantDataSet::Make3DRadiantDataSet(vtkm::IdComponent dim)
{
  // create a cube from -.5 to .5 in x,y,z, consisting of <dim> cells on each
  // axis, with point values equal to the Euclidean distance from the origin.

  vtkm::cont::DataSet dataSet;

  using HexTag = vtkm::CellShapeTagHexahedron;
  using HexTraits = vtkm::CellTraits<HexTag>;

  using CoordType = vtkm::Vec3f_32;

  const vtkm::IdComponent nCells = dim * dim * dim;

  vtkm::Float32 spacing = vtkm::Float32(1. / dim);
  CoordinateArrayHandle coordinates(vtkm::Id3(dim + 1, dim + 1, dim + 1),
                                    CoordType(-.5, -.5, -.5),
                                    CoordType(spacing, spacing, spacing));

  DataArrayHandle distanceToOrigin(coordinates);
  DataArrayHandle distanceToOther(coordinates, EuclideanNorm(CoordType(1., 1., 1.)));

  ConnectivityArrayHandle connectivity(
    vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, nCells * HexTraits::NUM_POINTS),
    CubeGridConnectivity(dim));

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(vtkm::cont::Field(
    "distanceToOrigin", vtkm::cont::Field::Association::POINTS, distanceToOrigin));
  dataSet.AddField(
    vtkm::cont::Field("distanceToOther", vtkm::cont::Field::Association::POINTS, distanceToOther));

  CellSet cellSet;
  cellSet.Fill(coordinates.GetNumberOfValues(), HexTag::Id, HexTraits::NUM_POINTS, connectivity);

  dataSet.SetCellSet(cellSet);

  return dataSet;
}

void TestContourUniformGrid()
{
  std::cout << "Testing Contour filter on a uniform grid" << std::endl;

  vtkm::Id3 dims(4, 4, 4);
  vtkm::source::Tangle tangle(dims);
  vtkm::cont::DataSet dataSet = tangle.Execute();

  vtkm::filter::Contour mc;

  mc.SetGenerateNormals(true);
  mc.SetIsoValue(0, 0.5);
  mc.SetActiveField("nodevar");
  mc.SetFieldsToPass(vtkm::filter::FieldSelection::MODE_NONE);

  auto result = mc.Execute(dataSet);
  {
    VTKM_TEST_ASSERT(result.GetNumberOfCoordinateSystems() == 1,
                     "Wrong number of coordinate systems in the output dataset");
    //since normals is on we have one field
    VTKM_TEST_ASSERT(result.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");
  }

  // let's execute with mapping fields.
  mc.SetFieldsToPass("nodevar");
  result = mc.Execute(dataSet);
  {
    const bool isMapped = result.HasField("nodevar");
    VTKM_TEST_ASSERT(isMapped, "mapping should pass");

    VTKM_TEST_ASSERT(result.GetNumberOfFields() == 2,
                     "Wrong number of fields in the output dataset");

    vtkm::cont::CoordinateSystem coords = result.GetCoordinateSystem();
    vtkm::cont::DynamicCellSet dcells = result.GetCellSet();
    using CellSetType = vtkm::cont::CellSetSingleType<>;
    const CellSetType& cells = dcells.Cast<CellSetType>();

    //verify that the number of points is correct (72)
    //verify that the number of cells is correct (160)
    VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 72,
                     "Should have less coordinates than the unmerged version");
    VTKM_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
  }

  //Now try with vertex merging disabled
  mc.SetMergeDuplicatePoints(false);
  mc.SetFieldsToPass(vtkm::filter::FieldSelection::MODE_ALL);
  result = mc.Execute(dataSet);
  {
    vtkm::cont::CoordinateSystem coords = result.GetCoordinateSystem();

    VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 480,
                     "Should have less coordinates than the unmerged version");

    //verify that the number of cells is correct (160)
    vtkm::cont::DynamicCellSet dcells = result.GetCellSet();

    using CellSetType = vtkm::cont::CellSetSingleType<>;
    const CellSetType& cells = dcells.Cast<CellSetType>();
    VTKM_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
  }
}

void TestContourCustomPolicy()
{
  std::cout << "Testing Contour filter with custom field and cellset" << std::endl;

  using DataSetGenerator = MakeRadiantDataSet;
  DataSetGenerator dataSetGenerator;

  const vtkm::IdComponent Dimension = 10;
  vtkm::cont::DataSet dataSet = dataSetGenerator.Make3DRadiantDataSet(Dimension);

  vtkm::filter::Contour mc;

  mc.SetGenerateNormals(false);
  mc.SetIsoValue(0, 0.45);
  mc.SetIsoValue(1, 0.45);
  mc.SetIsoValue(2, 0.45);
  mc.SetIsoValue(3, 0.45);

  //We specify a custom execution policy here, since the "distanceToOrigin" is a
  //custom field type
  mc.SetActiveField("distanceToOrigin");
  mc.SetFieldsToPass({ "distanceToOrigin", "distanceToOther" });
  vtkm::cont::DataSet outputData = mc.Execute(dataSet, PolicyRadiantDataSet{});

  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                   "Wrong number of fields in the output dataset");

  vtkm::cont::CoordinateSystem coords = outputData.GetCoordinateSystem();
  VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == (414 * 4), "Should have some coordinates");
}


vtkm::cont::DataSet MakeNormalsTestDataSet()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id3 dimensions(3, 4, 4);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
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
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);

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

  vtkm::cont::ArrayHandle<vtkm::Vec3f> normals;

  vtkm::filter::Contour mc;
  mc.SetIsoValue(0, 200);
  mc.SetGenerateNormals(true);

  // Test default normals generation: high quality for structured, fast for unstructured.
  auto expected = structured ? hq_sg : fast;

  mc.SetActiveField("pointvar");
  auto result = mc.Execute(dataset);
  result.GetField("normals").GetData().CopyTo(normals);
  VTKM_TEST_ASSERT(normals.GetNumberOfValues() == numVerts,
                   "Wrong number of values in normals field");
  for (vtkm::Id i = 0; i < numVerts; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(normals.GetPortalConstControl().Get(i), expected[i], 0.001),
                     "Result (",
                     normals.GetPortalConstControl().Get(i),
                     ") does not match expected value (",
                     expected[i],
                     ") vert ",
                     i);
  }

  // Test the other normals generation method
  if (structured)
  {
    mc.SetComputeFastNormalsForStructured(true);
    expected = fast;
  }
  else
  {
    mc.SetComputeFastNormalsForUnstructured(false);
    expected = hq_ug;
  }

  result = mc.Execute(dataset);
  result.GetField("normals").GetData().CopyTo(normals);
  VTKM_TEST_ASSERT(normals.GetNumberOfValues() == numVerts,
                   "Wrong number of values in normals field");
  for (vtkm::Id i = 0; i < numVerts; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(normals.GetPortalConstControl().Get(i), expected[i], 0.001),
                     "Result does not match expected values");
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

void TestContourFilter()
{
  TestContourUniformGrid();
  TestContourCustomPolicy();
  TestContourNormals();
}

} // anonymous namespace

int UnitTestContourFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(vtkm_ut_mc_filter::TestContourFilter, argc, argv);
}
