//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_StaticAssert_h
#define vtk_m_StaticAssert_h

#include <type_traits>
#include <vtkm/internal/Configure.h>

#define VTKM_STATIC_ASSERT(condition)                                                              \
  static_assert((condition), "Failed static assert: " #condition)
#define VTKM_STATIC_ASSERT_MSG(condition, message) static_assert((condition), message)

namespace vtkm
{

template <bool noError>
struct ReadTheSourceCodeHereForHelpOnThisError;

template <>
struct ReadTheSourceCodeHereForHelpOnThisError<true> : std::true_type
{
};

} // namespace vtkm

#define VTKM_READ_THE_SOURCE_CODE_FOR_HELP(noError)                                                \
  VTKM_STATIC_ASSERT(vtkm::ReadTheSourceCodeHereForHelpOnThisError<noError>::value)

#endif //vtk_m_StaticAssert_h
