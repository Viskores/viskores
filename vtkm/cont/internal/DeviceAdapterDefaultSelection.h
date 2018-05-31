//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

/* This file contains the logic to automatically determine and include a
 * default device adapter. */

#ifndef vtk_m_cont_internal_DeviceAdapterDefaultSelection_h
#define vtk_m_cont_internal_DeviceAdapterDefaultSelection_h

#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/internal/Configure.h>

//-----------------------------------------------------------------------------
// Select the default devices based on available options.
#ifndef VTKM_DEVICE_ADAPTER
#ifdef VTKM_CUDA
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_CUDA
#elif defined(VTKM_ENABLE_TBB) // !VTKM_CUDA
// Unfortunately, VTKM_ENABLE_TBB does not guarantee that TBB is (or isn't)
// available, but there is no way to check for sure in a header library.
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_TBB
#elif defined(VTKM_ENABLE_OPENMP)
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_OPENMP
#else // !VTKM_CUDA && !VTKM_ENABLE_TBB && !VTKM_ENABLE_OPENMP
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif // !VTKM_CUDA && !VTKM_ENABLE_TBB
#endif // VTKM_DEVICE_ADAPTER

//-----------------------------------------------------------------------------
// Bring in the appropriate device adapter tags for the default device:
//
// Serial:
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_SERIAL

#include <vtkm/cont/serial/internal/ArrayManagerExecutionSerial.h>
#include <vtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagSerial

// Cuda:
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA

#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagCuda

// TBB:
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB

#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagTBB

// OpenMP:
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP

#include <vtkm/cont/openmp/internal/ArrayManagerExecutionOpenMP.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterAlgorithmOpenMP.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagOpenMP

// Error:
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/internal/DeviceAdapterError.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagError

// Unknown:
#elif (VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_UNDEFINED) || !defined(VTKM_DEVICE_ADAPTER)

#ifndef VTKM_DEFAULT_DEVICE_ADAPTER_TAG
#warning If device adapter is undefined, VTKM_DEFAULT_DEVICE_ADAPTER_TAG must be defined.
#endif

#else

#warning Unrecognized device adapter given.

#endif

#endif // vtk_m_cont_internal_DeviceAdapterDefaultSelection_h
