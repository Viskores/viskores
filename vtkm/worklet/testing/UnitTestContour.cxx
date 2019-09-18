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
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/filter/ClipWithImplicitFunction.h>
#include <vtkm/source/Tangle.h>
#include <vtkm/worklet/Contour.h>

namespace vtkm_ut_mc_worklet
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

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, nCells),
                        cellFieldArray);

  ConnectivityArrayHandle connectivity(
    vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, nCells * HexTraits::NUM_POINTS),
    CubeGridConnectivity(dim));

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(vtkm::cont::Field(
    "distanceToOrigin", vtkm::cont::Field::Association::POINTS, distanceToOrigin));
  dataSet.AddField(vtkm::cont::Field("distanceToOther",
                                     vtkm::cont::Field::Association::POINTS,
                                     vtkm::cont::VariantArrayHandle(distanceToOther)));

  CellSet cellSet;
  cellSet.Fill((dim + 1) * (dim + 1) * (dim + 1), HexTag::Id, HexTraits::NUM_POINTS, connectivity);

  dataSet.SetCellSet(cellSet);

  dataSet.AddField(
    vtkm::cont::Field("cellvar", vtkm::cont::Field::Association::CELL_SET, cellFieldArray));

  return dataSet;
}

} // vtkm_ut_mc_worklet namespace

void TestContourUniformGrid()
{
  std::cout << "Testing Contour worklet on a uniform grid" << std::endl;

  vtkm::Id3 dims(4, 4, 4);
  vtkm::source::Tangle tangle(dims);
  vtkm::cont::DataSet dataSet = tangle.Execute();

  vtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);
  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  dataSet.GetField("nodevar").GetData().CopyTo(pointFieldArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  dataSet.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  vtkm::worklet::Contour isosurfaceFilter;
  isosurfaceFilter.SetMergeDuplicatePoints(false);

  vtkm::Float32 contourValue = 0.5f;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> verticesArray;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> normalsArray;
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;

  auto result = isosurfaceFilter.Run(&contourValue,
                                     1,
                                     cellSet,
                                     dataSet.GetCoordinateSystem(),
                                     pointFieldArray,
                                     verticesArray,
                                     normalsArray);

  scalarsArray = isosurfaceFilter.ProcessPointField(pointFieldArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = isosurfaceFilter.ProcessCellField(cellFieldArray);

  std::cout << "vertices: ";
  vtkm::cont::printSummary_ArrayHandle(verticesArray, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  vtkm::cont::printSummary_ArrayHandle(normalsArray, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  vtkm::cont::printSummary_ArrayHandle(scalarsArray, std::cout);
  std::cout << std::endl;
  std::cout << "cell field: ";
  vtkm::cont::printSummary_ArrayHandle(cellFieldArrayOut, std::cout);
  std::cout << std::endl;

  VTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues());

  VTKM_TEST_ASSERT(result.GetNumberOfCells() == 160);

  VTKM_TEST_ASSERT(verticesArray.GetNumberOfValues() == 480);
}

void TestContourExplicit()
{
  std::cout << "Testing Contour worklet on explicit data" << std::endl;

  using DataSetGenerator = vtkm_ut_mc_worklet::MakeRadiantDataSet;
  using Vec3Handle = vtkm::cont::ArrayHandle<vtkm::Vec3f_32>;
  using DataHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;

  DataSetGenerator dataSetGenerator;

  vtkm::IdComponent Dimension = 10;
  vtkm::Float32 contourValue = vtkm::Float32(.45);

  vtkm::cont::DataSet dataSet = dataSetGenerator.Make3DRadiantDataSet(Dimension);

  DataSetGenerator::CellSet cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::cont::Field contourField = dataSet.GetField("distanceToOrigin");
  DataSetGenerator::DataArrayHandle contourArray;
  contourField.GetData().CopyTo(contourArray);
  Vec3Handle vertices;
  Vec3Handle normals;

  vtkm::worklet::Contour Contour;
  Contour.SetMergeDuplicatePoints(false);

  auto result = Contour.Run(
    &contourValue, 1, cellSet, dataSet.GetCoordinateSystem(), contourArray, vertices, normals);

  DataHandle scalars;

  vtkm::cont::Field projectedField = dataSet.GetField("distanceToOther");

  DataSetGenerator::DataArrayHandle projectedArray;
  projectedField.GetData().CopyTo(projectedArray);

  scalars = Contour.ProcessPointField(projectedArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  dataSet.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = Contour.ProcessCellField(cellFieldArray);

  std::cout << "vertices: ";
  vtkm::cont::printSummary_ArrayHandle(vertices, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  vtkm::cont::printSummary_ArrayHandle(normals, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  vtkm::cont::printSummary_ArrayHandle(scalars, std::cout);
  std::cout << std::endl;
  std::cout << "cell field: ";
  vtkm::cont::printSummary_ArrayHandle(cellFieldArrayOut, std::cout);
  std::cout << std::endl;

  VTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues());
  VTKM_TEST_ASSERT(result.GetNumberOfCells() == 824);
  VTKM_TEST_ASSERT(test_equal(vertices.GetNumberOfValues(), 2472));
  VTKM_TEST_ASSERT(test_equal(normals.GetNumberOfValues(), 2472));
  VTKM_TEST_ASSERT(test_equal(scalars.GetNumberOfValues(), 2472));
}

void TestContourClipped()
{
  std::cout << "Testing Contour worklet on a clipped uniform grid" << std::endl;

  vtkm::Id3 dims(4, 4, 4);
  vtkm::source::Tangle tangle(dims);
  vtkm::cont::DataSet dataSet = tangle.Execute();

  vtkm::Plane plane(vtkm::make_Vec(0.51, 0.51, 0.51), vtkm::make_Vec(1, 1, 1));
  vtkm::filter::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(vtkm::cont::make_ImplicitFunctionHandle(plane));
  vtkm::cont::DataSet clipped = clip.Execute(dataSet);

  vtkm::cont::CellSetExplicit<> cellSet;
  clipped.GetCellSet().CopyTo(cellSet);
  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  clipped.GetField("nodevar").GetData().CopyTo(pointFieldArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  clipped.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  vtkm::Float32 contourValue = 0.5f;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> verticesArray;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> normalsArray;
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;

  vtkm::worklet::Contour isosurfaceFilter;
  isosurfaceFilter.SetMergeDuplicatePoints(false);

  auto result = isosurfaceFilter.Run(&contourValue,
                                     1,
                                     cellSet,
                                     clipped.GetCoordinateSystem(),
                                     pointFieldArray,
                                     verticesArray,
                                     normalsArray);

  scalarsArray = isosurfaceFilter.ProcessPointField(pointFieldArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = isosurfaceFilter.ProcessCellField(cellFieldArray);

  std::cout << "vertices: ";
  vtkm::cont::printSummary_ArrayHandle(verticesArray, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  vtkm::cont::printSummary_ArrayHandle(normalsArray, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  vtkm::cont::printSummary_ArrayHandle(scalarsArray, std::cout);
  std::cout << std::endl;
  std::cout << "cell field: ";
  vtkm::cont::printSummary_ArrayHandle(cellFieldArrayOut, std::cout);
  std::cout << std::endl;

  VTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues());
  VTKM_TEST_ASSERT(result.GetNumberOfCells() == 170);
  VTKM_TEST_ASSERT(verticesArray.GetNumberOfValues() == 510);
  VTKM_TEST_ASSERT(normalsArray.GetNumberOfValues() == 510);
  VTKM_TEST_ASSERT(scalarsArray.GetNumberOfValues() == 510);
}

void TestContour()
{
  TestContourUniformGrid();
  TestContourExplicit();
  TestContourClipped();
}

int UnitTestContour(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContour, argc, argv);
}
