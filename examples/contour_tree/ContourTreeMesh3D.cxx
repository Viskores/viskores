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

//We first check if VTKM_DEVICE_ADAPTER is defined, so that when TBB and CUDA
//includes this file we use the device adapter that they have set.
#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <vtkm/filter/ContourTreeUniform.h>

// Compute and render an isosurface for a uniform grid example
int main(int argc, char* argv[])
{
  std::cout << "ContourTreeMesh3D Example" << std::endl;

  if (argc != 2) {
    std::cout << "Parameter is fileName" << std::endl;
    std::cout << "File is expected to be ASCII with xdim ydim zdim integers " << std::endl;
    std::cout << "followed by vector data last dimension varying fastest" << std::endl;
    return 0;
  }

  // open input file
  ifstream inFile(argv[1]);
  if (inFile.bad()) return 0;

  // read size of mesh
  vtkm::Id3 vdims;
  inFile >> vdims[0];
  inFile >> vdims[1];
  inFile >> vdims[2];
  vtkm::Id nVertices = vdims[0] * vdims[1] * vdims[2];

  // read data
  vtkm::Float32 values[nVertices];
  for (int vertex = 0; vertex < nVertices; vertex++)
    inFile >> values[vertex];
  inFile.close();

  // build the input dataset
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet inDataSet = dsb.Create(vdims);

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(inDataSet, "values", values, nVertices);

  // Output data set is pairs of saddle and peak vertex IDs
  vtkm::filter::ResultField result;

  // Convert 3D mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreeMesh3D filter;
  result = filter.Execute(inDataSet, std::string("values"));

  vtkm::cont::Field resultField =  result.GetField();
  vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;
  resultField.GetData().CopyTo(saddlePeak);

  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif
