//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/IsosurfaceUniformGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexId, FieldOut<Scalar> v);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  const vtkm::Id xdim, ydim, zdim;
  const float xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT_EXPORT
  TangleField(const vtkm::Id3 dims, const float mins[3], const float maxs[3]) : xdim(dims[0]), ydim(dims[1]), zdim(dims[2]),
              xmin(mins[0]), ymin(mins[1]), zmin(mins[2]), xmax(maxs[0]), ymax(maxs[1]), zmax(maxs[2]), cellsPerLayer((xdim) * (ydim)) { };

  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Id &vertexId, vtkm::Float32 &v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const float fx = static_cast<float>(x) / static_cast<float>(xdim-1);
    const float fy = static_cast<float>(y) / static_cast<float>(xdim-1);
    const float fz = static_cast<float>(z) / static_cast<float>(xdim-1);

    const vtkm::Float32 xx = 3.0f*(xmin+(xmax-xmin)*(fx));
    const vtkm::Float32 yy = 3.0f*(ymin+(ymax-ymin)*(fy));
    const vtkm::Float32 zz = 3.0f*(zmin+(zmax-zmin)*(fz));

    v = (xx*xx*xx*xx - 5.0f*xx*xx + yy*yy*yy*yy - 5.0f*yy*yy + zz*zz*zz*zz - 5.0f*zz*zz + 11.8f) * 0.2f + 0.5f;
  }
};


vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  float mins[3] = {-1.0f, -1.0f, -1.0f};
  float maxs[3] = {1.0f, 1.0f, 1.0f};

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> vertexCountImplicitArray(0, vdims[0]*vdims[1]*vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, fieldArray);

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

  dataSet.AddField(vtkm::cont::Field("nodevar", 1, vtkm::cont::Field::ASSOC_POINTS, fieldArray));

  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

}


void TestIsosurfaceUniformGrid()
{
  std::cout << "Testing IsosurfaceUniformGrid Filter" << std::endl;

  vtkm::Id3 dims(4,4,4);
  vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  vtkm::worklet::IsosurfaceFilterUniformGrid<vtkm::Float32,
                                            DeviceAdapter> isosurfaceFilter(dims, dataSet);

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3> > verticesArray, normalsArray;
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;
  isosurfaceFilter.Run(0.5,
                       dataSet.GetField("nodevar").GetData(),
                       verticesArray,
                       normalsArray,
                       scalarsArray);

  VTKM_TEST_ASSERT(test_equal(verticesArray.GetNumberOfValues(), 480),
                   "Wrong result for Isosurface filter");
}

int UnitTestIsosurfaceUniformGrid(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestIsosurfaceUniformGrid);
}
