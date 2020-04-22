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

// Pick up conditions where we want to turn on/off assert.
#ifndef VTKM_NO_ASSERT
#if defined(NDEBUG)
#define VTKM_NO_ASSERT
#elif defined(VTKM_CUDA_DEVICE_PASS) && defined(VTKM_NO_ASSERT_CUDA)
#define VTKM_NO_ASSERT
#endif
#endif // VTKM_NO_ASSERT

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
#ifndef VTKM_NO_ASSERT
#define VTKM_ASSERT(condition) assert(condition)
#define VTKM_ASSERTS_CHECKED
#else
#define VTKM_ASSERT(condition) (void)(condition)
#endif

#endif //vtk_m_Assert_h
