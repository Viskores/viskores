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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_Assert_h
#define vtk_m_cont_Assert_h

#include <vtkm/cont/ErrorControlAssert.h>

// Stringify macros for VTKM_ASSERT_CONT
#define __VTKM_ASSERT_CONT_STRINGIFY_2ND(s) #s
#define __VTKM_ASSERT_CONT_STRINGIFY(s) __VTKM_ASSERT_CONT_STRINGIFY_2ND(s)

/// \def VTKM_ASSERT_CONT(condition)
///
/// Asserts that \a condition resolves to true.  If \a condition is false,
/// then an error is raised.  This macro is meant to work in the VTKm control
/// environment and throws an ErrorControlAssert object on failure.
///
/// Like the POSIX assert macro, the check will be removed when compiling
/// in non-debug mode (specifically when NDEBUG is defined), so be prepared
/// for the possibility that the condition is never evaluated.
///
/// VTKM_ASSERT_CONT will also be removed when compiling for CUDA devices.
/// Technically speaking, this macro should not be used in methods targeted for
/// CUDA devices since they run in the execution environment and this is for
/// the control environment. However, it is often convenient to have an assert
/// in a method that is to run in either control or execution environment, so
/// we go ahead and let you declare the assert there as well.
///
#if !defined(NDEBUG) && !defined(__CUDA_ARCH__)
#define VTKM_ASSERT_CONT(condition) \
  if (!(condition)) \
    ::vtkm::cont::Assert(condition, __FILE__, __LINE__, #condition)
#else
#define VTKM_ASSERT_CONT(condition)
#endif

namespace vtkm {
namespace cont {

VTKM_CONT_EXPORT void Assert(bool condition,
                             const std::string &file,
                             vtkm::Id line,
                             const std::string &message)
{
  if (condition)
  {
    // Do nothing.
  }
  else
  {
    throw vtkm::cont::ErrorControlAssert(file, line, message);
  }
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_Assert_h
