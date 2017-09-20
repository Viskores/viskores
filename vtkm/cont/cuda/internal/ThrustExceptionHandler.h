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
#ifndef vtk_m_cont_cuda_interal_ThrustExecptionHandler_h
#define vtk_m_cont_cuda_interal_ThrustExecptionHandler_h

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/internal/ExportMacros.h>

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
