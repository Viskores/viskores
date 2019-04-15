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

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/MarchingCubes.h>

namespace vtkm_ut_mc_worklet
{

class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn vertexId, FieldOut v);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  const vtkm::Id xdim, ydim, zdim;
  const vtkm::FloatDefault xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT
  TangleField(const vtkm::Id3 dims,
              const vtkm::FloatDefault mins[3],
              const vtkm::FloatDefault maxs[3])
    : xdim(dims[0])
    , ydim(dims[1])
    , zdim(dims[2])
    , xmin(mins[0])
    , ymin(mins[1])
    , zmin(mins[2])
    , xmax(maxs[0])
    , ymax(maxs[1])
    , zmax(maxs[2])
    , cellsPerLayer((xdim) * (ydim))
  {
  }

  VTKM_EXEC
  void operator()(const vtkm::Id& vertexId, vtkm::Float32& v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const vtkm::FloatDefault fx =
      static_cast<vtkm::FloatDefault>(x) / static_cast<vtkm::FloatDefault>(xdim - 1);
    const vtkm::FloatDefault fy =
      static_cast<vtkm::FloatDefault>(y) / static_cast<vtkm::FloatDefault>(xdim - 1);
    const vtkm::FloatDefault fz =
      static_cast<vtkm::FloatDefault>(z) / static_cast<vtkm::FloatDefault>(xdim - 1);

    const vtkm::Float32 xx = 3.0f * vtkm::Float32(xmin + (xmax - xmin) * (fx));
    const vtkm::Float32 yy = 3.0f * vtkm::Float32(ymin + (ymax - ymin) * (fy));
    const vtkm::Float32 zz = 3.0f * vtkm::Float32(zmin + (zmax - zmin) * (fz));

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};

vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  vtkm::FloatDefault mins[3] = { -1.0f, -1.0f, -1.0f };
  vtkm::FloatDefault maxs[3] = { 1.0f, 1.0f, 1.0f };

  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  vtkm::cont::ArrayHandleIndex vertexCountImplicitArray(vdims[0] * vdims[1] * vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(
    TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, pointFieldArray);

  vtkm::Id numCells = dims[0] * dims[1] * dims[2];
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, numCells),
                        cellFieldArray);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(1.0f / static_cast<vtkm::FloatDefault>(dims[0]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[2]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  static constexpr vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  dataSet.AddField(
    vtkm::cont::Field("nodevar", vtkm::cont::Field::Association::POINTS, pointFieldArray));
  dataSet.AddField(vtkm::cont::Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, "cells", cellFieldArray));

  return dataSet;
}

class EuclideanNorm
{
public:
  VTKM_EXEC_CONT
  EuclideanNorm()
    : Reference(0., 0., 0.)
  {
  }
  VTKM_EXEC_CONT
  EuclideanNorm(vtkm::Vec<vtkm::Float32, 3> reference)
    : Reference(reference)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Float32 operator()(vtkm::Vec<vtkm::Float32, 3> v) const
  {
    vtkm::Vec<vtkm::Float32, 3> d(
      v[0] - this->Reference[0], v[1] - this->Reference[1], v[2] - this->Reference[2]);
    return vtkm::Magnitude(d);
  }

private:
  vtkm::Vec<vtkm::Float32, 3> Reference;
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

  using CoordType = vtkm::Vec<vtkm::Float32, 3>;

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

  CellSet cellSet("cells");
  cellSet.Fill((dim + 1) * (dim + 1) * (dim + 1), HexTag::Id, HexTraits::NUM_POINTS, connectivity);

  dataSet.AddCellSet(cellSet);

  dataSet.AddField(vtkm::cont::Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, "cells", cellFieldArray));

  return dataSet;
}

} // vtkm_ut_mc_worklet namespace

void TestMarchingCubesUniformGrid()
{
  std::cout << "Testing MarchingCubes worklet on a uniform grid" << std::endl;

  vtkm::Id3 dims(4, 4, 4);
  vtkm::cont::DataSet dataSet = vtkm_ut_mc_worklet::MakeIsosurfaceTestDataSet(dims);

  vtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);
  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  dataSet.GetField("nodevar").GetData().CopyTo(pointFieldArray);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  dataSet.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  vtkm::worklet::MarchingCubes isosurfaceFilter;
  isosurfaceFilter.SetMergeDuplicatePoints(false);

  vtkm::Float32 contourValue = 0.5f;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> verticesArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> normalsArray;
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

  VTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues(),
                   "Output cell data invalid");

  VTKM_TEST_ASSERT(test_equal(verticesArray.GetNumberOfValues(), 480),
                   "Wrong result for Isosurface filter");
}

void TestMarchingCubesExplicit()
{
  std::cout << "Testing MarchingCubes worklet on explicit data" << std::endl;

  using DataSetGenerator = vtkm_ut_mc_worklet::MakeRadiantDataSet;
  using Vec3Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>;
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

  vtkm::worklet::MarchingCubes marchingCubes;
  marchingCubes.SetMergeDuplicatePoints(false);

  auto result = marchingCubes.Run(
    &contourValue, 1, cellSet, dataSet.GetCoordinateSystem(), contourArray, vertices, normals);

  DataHandle scalars;

  vtkm::cont::Field projectedField = dataSet.GetField("distanceToOther");

  DataSetGenerator::DataArrayHandle projectedArray;
  projectedField.GetData().CopyTo(projectedArray);

  scalars = marchingCubes.ProcessPointField(projectedArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  dataSet.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = marchingCubes.ProcessCellField(cellFieldArray);

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

  VTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues(),
                   "Output cell data invalid");
  VTKM_TEST_ASSERT(test_equal(vertices.GetNumberOfValues(), 2472),
                   "Wrong vertices result for MarchingCubes worklet");
  VTKM_TEST_ASSERT(test_equal(normals.GetNumberOfValues(), 2472),
                   "Wrong normals result for MarchingCubes worklet");
  VTKM_TEST_ASSERT(test_equal(scalars.GetNumberOfValues(), 2472),
                   "Wrong scalars result for MarchingCubes worklet");
}

void TestMarchingCubes()
{
  TestMarchingCubesUniformGrid();
  TestMarchingCubesExplicit();
}

int UnitTestMarchingCubes(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMarchingCubes, argc, argv);
}
