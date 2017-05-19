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

#ifdef VTKM_DEVICE_ADAPTER
#undef VTKM_DEVICE_ADAPTER
#endif
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/cuda/ArrayHandleCuda.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include <vtkm/cont/cuda/internal/testing/Testing.h>
#include <vtkm/cont/testing/TestingArrayHandles.h>

#include <vtkm/cont/Field.h>


//simple test to verify the array handle cuda compiles
void TestCudaHandle()
{
  //Verify that we can construct a cuda array handle using the class inside
  //the vtkm::cont::cuda namespace
  vtkm::cont::cuda::ArrayHandle<vtkm::Id> handleFoo(nullptr,0);
  vtkm::cont::Field foo("foo", vtkm::cont::Field::ASSOC_CELL_SET , "cellset", handleFoo);

  //Verify that we can construct a cuda array handle using the class inside
  //the vtkm::cont namespace
  vtkm::cont::ArrayHandleCuda< vtkm::Vec< vtkm::Float32, 3> > handleBar(nullptr,0);
  vtkm::cont::Field bar("bar", vtkm::cont::Field::ASSOC_CELL_SET, "cellset", handleBar);
}


int UnitTestCudaArrayHandle(int, char *[])
{
  TestCudaHandle();

  int result = vtkm::cont::testing::TestingArrayHandles
      <vtkm::cont::DeviceAdapterTagCuda>::Run();
  return vtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);


}
