//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//This header can be used by external application that are consuming VTKm
//to define if VTKm should be set to use 64bit data types. If you need to
//customize more of the vtkm type system, or what Device Adapters
//need to be included look at vtkm/internal/Configure.h for all defines that
//you can over-ride.
#ifdef vtk_m_internal_Configure_h
#error Incorrect header order. Include this header before any other VTKm headers.
#endif

#ifndef vtk_m_internal_Configure32_h
#define vtk_m_internal_Configure32_h

#define VTKM_USE_DOUBLE_PRECISION
#define VTKM_USE_64BIT_IDS

#include <vtkm/internal/Configure.h>

#endif
