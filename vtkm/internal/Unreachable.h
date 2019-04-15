//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Unreachable_h
#define vtk_m_Unreachable_h

/// VTKM_UNREACHABLE is similar to VTK_ASSUME, with the significant difference
/// that it is not conditional. Control should never reach a path containing
/// a VTKM_UNREACHABLE statement under any circumstances.
///
/// If assertions are enabled (e.g. neither NDEBUG nor VTKM_NO_ASSERT is
/// defined), the following steps are taken:
/// 1. Print an error message containing the macro argument and location of the
///    VTKM_UNREACHABLE call.
/// 2. Abort the kernel (if CUDA) or process.
///
/// This allows bad code paths to be identified during development and
/// debugging.
///
/// If assertions are disabled and the compiler has some sort of 'unreachable'
/// intrinsic used to provide optimization hints, the intrinsic is used to
/// notify the compiler that this is a dead code path.
///
#define VTKM_UNREACHABLE(msg)                                                                      \
  VTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    VTKM_UNREACHABLE_IMPL();                                                                       \
    VTKM_UNREACHABLE_PRINT(msg);                                                                   \
    VTKM_UNREACHABLE_ABORT();                                                                      \
  }                                                                                                \
  VTKM_SWALLOW_SEMICOLON_POST_BLOCK

// VTKM_UNREACHABLE_IMPL is compiler-specific:
#if defined(VTKM_CUDA_DEVICE_PASS)

#define VTKM_UNREACHABLE_IMPL() (void)0 /* no-op, no known intrinsic */

#if defined(NDEBUG) || defined(VTKM_NO_ASSERT)

#define VTKM_UNREACHABLE_PRINT(msg) (void)0 /* no-op */
#define VTKM_UNREACHABLE_ABORT() (void)0    /* no-op */

#else // NDEBUG || VTKM_NO_ASSERT

#define VTKM_UNREACHABLE_PRINT(msg)                                                                \
  printf("Unreachable location reached: %s\nLocation: %s:%d\n", msg, __FILE__, __LINE__)
#define VTKM_UNREACHABLE_ABORT()                                                                   \
  asm("trap;") /* Triggers kernel exit with CUDA error 73: Illegal inst */

#endif // NDEBUG || VTKM_NO_ASSERT

#else // !CUDA


#if defined(NDEBUG) || defined(VTKM_NO_ASSERT)

#define VTKM_UNREACHABLE_PRINT(msg) (void)0 /* no-op */
#define VTKM_UNREACHABLE_ABORT() (void)0    /* no-op */

#if defined(VTKM_MSVC)
#define VTKM_UNREACHABLE_IMPL() __assume(false)
#elif defined(VTKM_ICC) && !defined(__GNUC__)
#define VTKM_UNREACHABLE_IMPL() __assume(false)
#elif (defined(VTKM_GCC) || defined(VTKM_ICC)) &&                                                  \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
// Added in 4.5.0:
#define VTKM_UNREACHABLE_IMPL() __builtin_unreachable()
#elif defined(VTKM_CLANG)
#define VTKM_UNREACHABLE_IMPL() __builtin_unreachable()
#else
#define VTKM_UNREACHABLE_IMPL() (void)0 /* no-op */
#endif

#else // NDEBUG || VTKM_NO_ASSERT

#define VTKM_UNREACHABLE_IMPL() (void)0
#define VTKM_UNREACHABLE_PRINT(msg)                                                                \
  std::cerr << "Unreachable location reached: " << msg << "\n"                                     \
            << "Location: " << __FILE__ << ":" << __LINE__ << "\n"
#define VTKM_UNREACHABLE_ABORT() abort()

#endif // NDEBUG && !VTKM_NO_ASSERT

#endif // !CUDA

#endif //vtk_m_Unreachable_h
