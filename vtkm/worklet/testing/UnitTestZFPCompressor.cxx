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

#include <vtkm/worklet/ZFPCompressor.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <iostream>

using Handle64 = vtkm::cont::ArrayHandle<vtkm::Float64>;

template <typename Device>
void Test3D()
{
  std::cout << "Testing ZFP 3d:" << std::endl;
  //vtkm::Id3 dims(4,4,4);
  vtkm::Id3 dims(8, 8, 8);
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();
  ;

  vtkm::worklet::ZFPCompressor<Device> compressor;

  vtkm::Float64 rate = 10;
  if (dynField.IsSameType(Handle64()))
  {
    Handle64 array = dynField.Cast<Handle64>();
    //std::cout<<"\n";
    for (int i = 0; i < 64; ++i)
    {
      std::cout << array.GetPortalControl().Get(i) << " ";
    }
    std::cout << "\n";
    compressor.Compress(array, rate, dims);
  }
}

void TestZFP()
{
  Test3D<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>();
}

int UnitTestZFPCompressor(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestZFP);
}
