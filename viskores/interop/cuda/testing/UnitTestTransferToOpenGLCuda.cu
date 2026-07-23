//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


//This sets up testing with the cuda device adapter
#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/cuda/internal/testing/Testing.h>

#include <viskores/interop/testing/TestingOpenGLInterop.h>

int UnitTestTransferToOpenGLCuda(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv);
  int result = 1;
  result = viskores::interop::testing::TestingOpenGLInterop<
    viskores::cont::cuda::DeviceAdapterTagCuda>::Run();
  return viskores::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}
