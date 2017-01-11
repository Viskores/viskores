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
#define VTKM_EXEC __device__ __host__
#define VTKM_EXEC_CONT __device__ __host__
#if __CUDAVER__ >= 75000
#  define VTKM_SUPPRESS_EXEC_WARNINGS #pragma nv_exec_check_disable
#else
#  define VTKM_SUPPRESS_EXEC_WARNINGS #pragma hd_warning_disable
#endif
#define VTKM_EXEC_CONSTANT __device__ __constant__
#else
#define VTKM_EXEC
#define VTKM_EXEC_CONT
#define VTKM_SUPPRESS_EXEC_WARNINGS
#define VTKM_EXEC_CONSTANT
#endif

#define VTKM_CONT

// constexpr support was added to VisualStudio 2015 and above. So this makes
// sure when that we gracefully fall back to just const when using 2013
#if defined(VTKM_MSVC) && _MSC_VER < 1900
#define VTKM_CONSTEXPR const
#define VTKM_NOEXCEPT
#else
#define VTKM_CONSTEXPR constexpr
#define VTKM_NOEXCEPT noexcept
#endif

#define VTKM_OVERRIDE override

/// Simple macro to identify a parameter as unused. This allows you to name a
/// parameter that is not used. There are several instances where you might
/// want to do this. For example, when using a parameter to overload or
/// template a function but do not actually use the parameter. Another example
/// is providing a specialization that does not need that parameter.
#define vtkmNotUsed(parameter_name)

#endif //vtk_m_internal__ExportMacros_h
