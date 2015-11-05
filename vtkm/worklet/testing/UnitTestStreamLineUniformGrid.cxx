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
#include <vtkm/cont/testing/Testing.h>

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

  vtkm::Id g_num_seeds = 25;
  vtkm::Id g_max_steps = 2000;
  vtkm::Id g_dim[3];
  int dim[3];

  // Read in the vector data for testing
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
  std::vector<vtkm::Vec<vtkm::Float32, 3> > field;

  FILE * pFile = fopen("/home/pkf/VTKM/VTKM-Fasel/vtk-m/vtkm/worklet/testing/tornado.vec", "rb");
  if (pFile == NULL) perror ("Error opening file");

  fread(dim, sizeof(int), 3, pFile);
  for (vtkm::Id i = 0; i < 3; i++)
  {
    g_dim[i] = static_cast<vtkm::Id>(dim[i]);
  }
  vtkm::Id num_elements = g_dim[0] * g_dim[1] * g_dim[2] * 3;
  std::cout << "Dimension of the data: " << g_dim[0] << "," << g_dim[1] << "," << g_dim[2] << std::endl;

  float* data = new float[num_elements];
  fread(data, sizeof(float), num_elements, pFile);
  for (vtkm::Id i = 0; i < num_elements; i++)
  {
    vtkm::Float32 x = data[i];
    vtkm::Float32 y = data[++i];
    vtkm::Float32 z = data[++i];
    vtkm::Vec<vtkm::Float32, 3> vec_data(x, y, z);
    field.push_back(Normalize(vec_data));
  }
  fieldArray = vtkm::cont::make_ArrayHandle(&field[0], field.size());

  // Make the output array
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > streamLineLists;

  // Create and run the filter
  vtkm::worklet::StreamLineUniformGridFilter<vtkm::Float32, DeviceAdapter>
                 streamLineUniformGridFilter(g_dim, g_num_seeds, g_max_steps); 

  streamLineUniformGridFilter.Run(0.5f, fieldArray, streamLineLists);
}

int UnitTestStreamLineUniformGrid(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamLineUniformGrid);
}
