//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Assert_h
#define vtk_m_Assert_h

#include <vtkm/internal/Configure.h>

#include <cassert>

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
/// The VTKM_NO_ASSERT cmake and preprocessor option allows debugging builds
/// to remove assertions for performance reasons.
#if defined(VTKM_CUDA_VERSION_MAJOR) && (VTKM_CUDA_VERSION_MAJOR == 7)
//CUDA 7.5 doesn't support assert in device code
#define VTKM_ASSERT(condition) (void)(condition)
#elif !defined(NDEBUG) && !defined(VTKM_NO_ASSERT)
//Only assert if we are in debug mode and don't have VTKM_NO_ASSERT defined
#define VTKM_ASSERT(condition) assert(condition)
#else
#define VTKM_ASSERT(condition) (void)(condition)
#endif

#endif //vtk_m_Assert_h
