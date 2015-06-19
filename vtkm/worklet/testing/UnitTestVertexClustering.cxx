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

#include <iostream>
#include <algorithm>

#include <vtkm/worklet/PointElevation.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/worklet/VertexClustering.h>
namespace{

void TestVertexClustering()
{
  double bounds[6];
  const int divisions = 4;
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet ds = maker.Make3DExplicitDataSetCowNose(bounds);

  // run
  vtkm::cont::DataSet ds_out = vtkm::worklet::VertexClustering<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().run(ds, bounds, divisions);

  // test

}
} // namespace

int UnitTestVertexClustering(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering);
}

