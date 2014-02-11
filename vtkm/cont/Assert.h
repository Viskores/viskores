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
#ifndef vtkm_cont_Assert_h
#define vtkm_cont_Assert_h

#include <vtkm/cont/ErrorControlAssert.h>

// Stringify macros for VTKM_ASSERT_CONT
#define __VTKM_ASSERT_CONT_STRINGIFY_2ND(s) #s
#define __VTKM_ASSERT_CONT_STRINGIFY(s) __VTKM_ASSERT_CONT_STRINGIFY_2ND(s)

/// \def VTKM_ASSERT_CONT(condition)
///
/// Asserts that \a condition resolves to true.  If \a condition is false,
/// then an error is raised.  This macro is meant to work in the VTKm control
/// environment and throws an ErrorControlAssert object on failure.

#ifndef NDEBUG
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

#endif //vtkm_cont_Assert_h
