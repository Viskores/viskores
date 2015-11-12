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

#include <vtkm/worklet/StreamLineUniformGrid.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <fstream>
#include <vector>
#include <math.h>

namespace {

template <typename T>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T,3> Normalize(vtkm::Vec<T,3> v)
{
  T magnitude = static_cast<T>(sqrt(vtkm::dot(v, v)));
  T zero = static_cast<T>(0.0);
  T one = static_cast<T>(1.0);
  if (magnitude == zero)
    return vtkm::make_Vec(zero, zero, zero);
  else
    return one / magnitude * v;
}

}

void TestStreamLineUniformGrid()
{
  std::cout << "Testing StreamLineUniformGrid Filter" << std::endl;

  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Parameters for streamlines
  vtkm::Id numSeeds = 25;
  vtkm::Id maxSteps = 2000;
  vtkm::Float32 timeStep = 0.5f;

  // Read in the vector data for testing
  FILE * pFile = fopen("/home/pkf/VTKM/VTKM-Fasel/vtk-m/vtkm/worklet/testing/tornado.vec", "rb");
  if (pFile == NULL) perror ("Error opening file");

  // Size of the dataset
  int dims[3];
  fread(dims, sizeof(int), 3, pFile);
  const vtkm::Id3 vdims(dims[0], dims[1], dims[2]);
  vtkm::Id nElements = vdims[0] * vdims[1] * vdims[2] * 3;

  // Read vector data at each point of the uniform grid and store
  float* data = new float[nElements];
  fread(data, sizeof(float), nElements, pFile);

  std::vector<vtkm::Vec<vtkm::Float32, 3> > field;
  for (vtkm::Id i = 0; i < nElements; i++)
  {
    vtkm::Float32 x = data[i];
    vtkm::Float32 y = data[++i];
    vtkm::Float32 z = data[++i];
    vtkm::Vec<vtkm::Float32, 3> vecData(x, y, z);
    field.push_back(Normalize(vecData));
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(&field[0], field.size());

  // Construct the input dataset (uniform) to hold the input and set vector data
  vtkm::cont::DataSet inDataSet;
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
  inDataSet.AddCoordinateSystem(
            vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));
  inDataSet.AddField(vtkm::cont::Field("vecData", 1, vtkm::cont::Field::ASSOC_POINTS, fieldArray));

  vtkm::cont::CellSetStructured<3> inCellSet("cells");
  inCellSet.SetPointDimensions(vtkm::make_Vec(vdims[0], vdims[1], vdims[2]));
  inDataSet.AddCellSet(inCellSet);

  // Construct the output dataset (explicit)
  vtkm::cont::DataSet outDataSet;
  vtkm::cont::CellSetExplicit<> outCellSet(numSeeds * maxSteps * 2, "cells", 3);
  outDataSet.AddCellSet(outCellSet);

  // Create and run the filter
  vtkm::worklet::StreamLineUniformGridFilter<vtkm::Float32, DeviceAdapter>
                 streamLineUniformGridFilter(inDataSet,
                                             outDataSet,
                                             vtkm::worklet::internal::BACKWARD,
                                             numSeeds, 
                                             maxSteps, 
                                             timeStep);

  streamLineUniformGridFilter.Run();
}

int UnitTestStreamLineUniformGrid(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamLineUniformGrid);
}
