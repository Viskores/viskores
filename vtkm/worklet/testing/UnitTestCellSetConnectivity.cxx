//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/MarchingCubes.h>

#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/worklet/connectivities/CellSetConnectivity.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

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

static vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
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


class TestCellSetConnectivity
{
public:
  void TestTangleIsosurface() const
  {
    vtkm::Id3 dims(4, 4, 4);
    vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

    vtkm::filter::MarchingCubes filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.1);
    filter.SetActiveField("nodevar");
    vtkm::cont::DataSet outputData = filter.Execute(dataSet);

    auto cellSet = outputData.GetCellSet().Cast<vtkm::cont::CellSetSingleType<>>();
    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    vtkm::worklet::connectivity::CellSetConnectivity().Run(cellSet, componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 8,
                     "Wrong number of connected components");
  }

  void TestExplicitDataSet() const
  {
    vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

    auto cellSet = dataSet.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    vtkm::worklet::connectivity::CellSetConnectivity().Run(cellSet, componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void TestUniformDataSet() const
  {
    vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();

    auto cellSet = dataSet.GetCellSet();
    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    vtkm::worklet::connectivity::CellSetConnectivity().Run(cellSet, componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void operator()() const
  {
    this->TestTangleIsosurface();
    this->TestExplicitDataSet();
    this->TestUniformDataSet();
  }
};

int UnitTestCellSetConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetConnectivity(), argc, argv);
}
