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
#ifndef vtk_m_internal__ExportMacros_h
#define vtk_m_internal__ExportMacros_h

#include <vtkm/internal/Configure.h>

/*!
  * Export macros for various parts of the VTKm library.
  */

#ifdef VTKM_CUDA
#define VTKM_EXEC_EXPORT inline __device__ __host__
#define VTKM_EXEC_CONT_EXPORT inline __device__ __host__
#define VTKM_SUPPRESS_EXEC_WARNINGS \
  #pragma hd_warning_disable \
  #pragma nv_exec_check_disable
#define VTKM_EXEC_CONSTANT_EXPORT __device__ __constant__
#else
#define VTKM_EXEC_EXPORT inline
#define VTKM_EXEC_CONT_EXPORT inline
#define VTKM_SUPPRESS_EXEC_WARNINGS
#define VTKM_EXEC_CONSTANT_EXPORT
#endif

#define VTKM_CONT_EXPORT inline

/// Simple macro to identify a parameter as unused. This allows you to name a
/// parameter that is not used. There are several instances where you might
/// want to do this. For example, when using a parameter to overload or
/// template a function but do not actually use the parameter. Another example
/// is providing a specialization that does not need that parameter.
#define vtkmNotUsed(parameter_name)


// Check boost support under CUDA
#ifdef VTKM_CUDA
#if !defined(BOOST_SP_DISABLE_THREADS) && !defined(BOOST_SP_USE_SPINLOCK) && !defined(BOOST_SP_USE_PTHREADS)
#warning -------------------------------------------------------------------
#warning The CUDA compiler (nvcc) has trouble with some of the optimizations
#warning boost uses for thread saftey.  To get around this, please define
#warning one of the following macros to specify the thread handling boost
#warning should use:
#warning
#warning   BOOST_SP_DISABLE_THREADS
#warning   BOOST_SP_USE_SPINLOCK
#warning   BOOST_SP_USE_PTHREADS
#warning
#warning Failure to define one of these for a CUDA build will probably cause
#warning other annoying warnings and might even cause incorrect code.  Note
#warning that specifying BOOST_SP_DISABLE_THREADS does not preclude using
#warning VTKm with a threaded device (like OpenMP).  Specifying one of these
#warning modes for boost does not effect the scheduling in VTKm.
#warning -------------------------------------------------------------------

#endif
#endif

#endif //vtk_m_internal__ExportMacros_h
