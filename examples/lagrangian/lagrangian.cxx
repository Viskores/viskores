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

#include "ABCfield.h"
#include <iostream>
#include <vector>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/filter/Lagrangian.h>

using namespace std;
#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

vtkm::cont::DataSet make3DRectilinearDataSet(double time)
{
  ABCfield field;

  double xmin, xmax, ymin, ymax, zmin, zmax;
  xmin = 0.0;
  ymin = 0.0;
  zmin = 0.0;

  xmax = 6.28;
  ymax = 6.28;
  zmax = 6.28;

  int dims[3] = { 16, 16, 16 };

  vtkm::cont::DataSetBuilderUniform dsb;

  double xdiff = (xmax - xmin) / (dims[0] - 1);
  double ydiff = (ymax - ymin) / (dims[1] - 1);
  double zdiff = (zmax - zmin) / (dims[2] - 1);

  vtkm::Id3 DIMS(dims[0], dims[1], dims[2]);
  vtkm::Vec<vtkm::Float64, 3> ORIGIN(0, 0, 0);
  vtkm::Vec<vtkm::Float64, 3> SPACING(xdiff, ydiff, zdiff);

  vtkm::cont::DataSet dataset = dsb.Create(DIMS, ORIGIN, SPACING);
  vtkm::cont::DataSetFieldAdd dsf;

  int numPoints = dims[0] * dims[1] * dims[2];

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> velocityField;
  velocityField.Allocate(numPoints);

  int count = 0;
  for (int i = 0; i < dims[0]; i++)
  {
    for (int j = 0; j < dims[1]; j++)
    {
      for (int k = 0; k < dims[2]; k++)
      {
        double vec[3];
        double loc[3] = { i * xdiff + xmin, j * ydiff + ymax, k * zdiff + zmin };
        field.calculateVelocity(loc, time, vec);
        velocityField.GetPortalControl().Set(count,
                                             vtkm::Vec<vtkm::Float64, 3>(vec[0], vec[1], vec[2]));
        count++;
      }
    }
  }
  dsf.AddPointField(dataset, "velocity", velocityField);
  return dataset;
}


int main()
{
  vtkm::filter::Lagrangian lagrangianFilter;
  lagrangianFilter.SetResetParticles(true);
  vtkm::Float32 stepSize = 0.01f;
  lagrangianFilter.SetStepSize(stepSize);
  lagrangianFilter.SetWriteFrequency(10);
  for (int i = 0; i < 100; i++)
  {
    vtkm::cont::DataSet inputData = make3DRectilinearDataSet((double)i * stepSize);
    lagrangianFilter.SetActiveField("velocity");
    vtkm::cont::DataSet extractedBasisFlows = lagrangianFilter.Execute(inputData);
  }
  return 0;
}
