//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_Assert_h
#define vtk_m_exec_Assert_h

#include <vtkm/internal/ExportMacros.h>

// Stringify macros for DAX_ASSERT_EXEC
#define __VTKM_ASSERT_EXEC_STRINGIFY_2ND(s) #s
#define __VTKM_ASSERT_EXEC_STRINGIFY(s) __VTKM_ASSERT_EXEC_STRINGIFY_2ND(s)

/// \def VTKM_ASSERT_EXEC(condition, work)
///
/// Asserts that \a condition resolves to true. If \a condition is false, then
/// an error is raised. This macro is meant to work in the VTK-m execution
/// environment and requires the \a work object to raise the error and throw it
/// in the control environment.
///
#ifndef NDEBUG
#define VTKM_ASSERT_EXEC(condition, work) \
  if (!(condition)) \
    ::vtkm::exec::Assert( \
        condition, \
        __FILE__ ":" __VTKM_ASSERT_EXEC_STRINGIFY(__LINE__) ": " \
        "Assert Failed (" #condition ")", \
        work)
#else
//in release mode we just act like we use the result of the condition
//and the worklet so that we don't introduce new issues.
#define VTKM_ASSERT_EXEC(condition, work) \
  (void)(condition); \
  (void)(work);
#endif

namespace vtkm {
namespace exec {

/// Implements the assert functionality of VTKM_ASSERT_EXEC.
///
template<typename WorkType>
VTKM_EXEC_EXPORT
void Assert(bool condition, const char *message, WorkType work)
{
  if (condition)
    {
    // Do nothing.
    }
  else
    {
    work.RaiseError(message);
    }
}

}
} // namespace vtkm::exec

#endif //vtk_m_exec_Assert_h
