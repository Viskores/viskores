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
#ifndef vtk_m_cont_cuda_internal_Testing_h
#define vtk_m_cont_cuda_internal_Testing_h

#include <vtkm/cont/testing/Testing.h>

#include <cuda.h>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

struct Testing
{
public:
  static VTKM_CONT int CheckCudaBeforeExit(int result)
  {
    cudaError_t cudaError = cudaPeekAtLastError();
    if (cudaError != cudaSuccess)
    {
      std::cout << "***** Unchecked Cuda error." << std::endl
                << cudaGetErrorString(cudaError) << std::endl;
      return 1;
    }
    else
    {
      std::cout << "No Cuda error detected." << std::endl;
    }
    return result;
  }

  template <class Func>
  static VTKM_CONT int Run(Func function)
  {
    int result = vtkm::cont::testing::Testing::Run(function);
    return CheckCudaBeforeExit(result);
  }
};
}
}
}
} // namespace vtkm::cont::cuda::internal

#endif //vtk_m_cont_cuda_internal_Testing_h
