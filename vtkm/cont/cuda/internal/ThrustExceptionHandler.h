//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_interal_ThrustExecptionHandler_h
#define vtk_m_cont_cuda_interal_ThrustExecptionHandler_h

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system_error.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

static inline void throwAsVTKmException()
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (std::bad_alloc& error)
  {
    throw vtkm::cont::ErrorBadAllocation(error.what());
  }
  catch (thrust::system_error& error)
  {
    throw vtkm::cont::ErrorExecution(error.what());
  }
}
}
}
}
}

#endif //vtk_m_cont_cuda_interal_ThrustExecptionHandler_h
