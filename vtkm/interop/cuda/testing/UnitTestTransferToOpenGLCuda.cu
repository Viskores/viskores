//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

//This sets up testing with the cuda device adapter
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/cuda/internal/testing/Testing.h>

#include <vtkm/interop/testing/TestingOpenGLInterop.h>

int UnitTestTransferToOpenGLCuda(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv);
  int result = 1;
  result =
    vtkm::interop::testing::TestingOpenGLInterop<vtkm::cont::cuda::DeviceAdapterTagCuda>::Run();
  return vtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}
