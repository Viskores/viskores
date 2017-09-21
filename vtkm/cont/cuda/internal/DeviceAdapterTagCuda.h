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
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h

#include <vtkm/cont/internal/DeviceAdapterTag.h>

//We always create the cuda tag when included, but we only mark it as
//a valid tag when VTKM_CUDA is true. This is for easier development
//of multi-backend systems
#ifdef VTKM_CUDA
VTKM_VALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#else
VTKM_INVALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#endif

#endif //vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h
