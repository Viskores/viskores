//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_Assert_h
#define vtk_m_Assert_h

#include <vtkm/internal/Configure.h>

#include <assert.h>

/// \def VTKM_ASSERT(condition)
///
/// Asserts that \a condition resolves to true.  If \a condition is false,
/// then a diagnostic message is outputted and execution is terminated. The
/// behavior is essentially the same as the POSIX assert macro, but is
/// wrapped for added portability.
///
/// Like the POSIX assert macro, the check will be removed when compiling
/// in non-debug mode (specifically when NDEBUG is defined), so be prepared
/// for the possibility that the condition is never evaluated.
///
#if !defined(NDEBUG)
#define VTKM_ASSERT(condition) \
  assert(condition)
#else
#define VTKM_ASSERT(condition)
#endif


#endif //vtk_m_Assert_h
