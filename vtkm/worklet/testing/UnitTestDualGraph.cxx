//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/CellEdge.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/worklet/connectivities/CellSetDualGraph.h>

class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexId, FieldOut<Scalar> v);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

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
  auto cellFieldArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, numCells);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(1.0f / static_cast<vtkm::FloatDefault>(dims[0]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[2]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  dataSet.AddField(vtkm::cont::Field("nodevar", vtkm::cont::Field::ASSOC_POINTS, pointFieldArray));
  dataSet.AddField(
    vtkm::cont::Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellFieldArray));

  return dataSet;
}


template <typename DeviceAdapter>
class TestDualGraph
{
public:
  void TestTriangleMesh() const
  {
    // cell2vertices connectivityusing Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    //std::vector<vtkm::Id> connectivity = {0, 2, 1, 1, 2, 4, 1, 4, 3, 2, 5, 4};
    std::vector<vtkm::Id> connectivity = { 0, 2, 4, 1, 3, 5, 2, 6, 4, 5, 3, 7, 2, 9, 6, 4, 6, 8 };

    vtkm::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(8, vtkm::CELL_SHAPE_TRIANGLE, 3, vtkm::cont::make_ArrayHandle(connectivity));

    vtkm::cont::ArrayHandle<vtkm::Id> numIndicesArray;
    vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetArray;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;

    CellSetDualGraph<DeviceAdapter>().Run(
      cellSet, numIndicesArray, indexOffsetArray, connectivityArray);
    std::cout << "numIndices: ";
    for (int i = 0; i < numIndicesArray.GetNumberOfValues(); i++)
    {
      std::cout << numIndicesArray.GetPortalConstControl().Get(i) << " ";
    }
    std::cout << std::endl;

    std::cout << "indexOffset: ";
    for (int i = 0; i < indexOffsetArray.GetNumberOfValues(); i++)
    {
      std::cout << indexOffsetArray.GetPortalConstControl().Get(i) << " ";
    }
    std::cout << std::endl;

    std::cout << "connectivity: ";
    for (int i = 0; i < connectivityArray.GetNumberOfValues(); i++)
    {
      std::cout << connectivityArray.GetPortalConstControl().Get(i) << " ";
    }
    std::cout << std::endl;
  }

  void TestIsosurface() const
  {
    vtkm::Id3 dims(3, 3, 3);
    vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

    vtkm::filter::MarchingCubes filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.5);
    vtkm::filter::Result result = filter.Execute(dataSet, dataSet.GetField("nodevar"));
    vtkm::cont::DataSet& outputData = result.GetDataSet();

    auto cellSet = outputData.GetCellSet().Cast<vtkm::cont::CellSetSingleType<>>();
    vtkm::cont::ArrayHandle<vtkm::Id> numIndicesArray;
    vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetArray;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;

    CellSetDualGraph<DeviceAdapter>().Run(
      cellSet, numIndicesArray, indexOffsetArray, connectivityArray);
    std::cout << "numIndices: ";
    for (int i = 0; i < numIndicesArray.GetNumberOfValues(); i++)
    {
      std::cout << numIndicesArray.GetPortalConstControl().Get(i) << " ";
    }
    std::cout << std::endl;

    std::cout << "indexOffset: ";
    for (int i = 0; i < indexOffsetArray.GetNumberOfValues(); i++)
    {
      std::cout << indexOffsetArray.GetPortalConstControl().Get(i) << " ";
    }
    std::cout << std::endl;

    std::cout << "connectivity: ";
    for (int i = 0; i < connectivityArray.GetNumberOfValues(); i++)
    {
      std::cout << connectivityArray.GetPortalConstControl().Get(i) << " ";
    }
    std::cout << std::endl;
  }
  void operator()() const
  {
    //this->TestTriangleMesh();
    this->TestIsosurface();
  }
};

int UnitTestDualGraph(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestDualGraph<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}