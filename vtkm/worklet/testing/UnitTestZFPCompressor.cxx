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

template <typename Scalar>
void Test3D(int rate)
{
  std::cout << "Testing ZFP 3d:" << std::endl;
  //vtkm::Id3 dims(4,4,4);
  //vtkm::Id3 dims(4,4,7);
  vtkm::Id3 dims(8, 8, 8);
  //vtkm::Id3 dims(256,256,256);
  //vtkm::Id3 dims(128,128,128);
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();
  ;

  vtkm::worklet::ZFPCompressor compressor;

  if (dynField.IsSameType(Handle64()))
  {
    Handle64 field = dynField.Cast<Handle64>();
    vtkm::cont::ArrayHandle<Scalar> handle;
    const vtkm::Id size = field.GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = field.GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (vtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }
    compressor.Compress(handle, rate, dims);
  }
}

void TestZFP()
{
  //Test3D<vtkm::Float64>(4);
  Test3D<vtkm::Float32>(4);
  //Test3D<vtkm::Int64>(4);
  //Test3D<vtkm::Int32>(4);
}

int UnitTestZFPCompressor(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestZFP);
}
