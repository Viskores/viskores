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
#ifndef vtk_m_StaticAssert_h
#define vtk_m_StaticAssert_h

#include <vtkm/internal/Configure.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/static_assert.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

// Newer versions of clang are causing the static assert to issue a warning
// about unused typedefs. In this case, we want to disable the warnings using
// pragmas. However, not all compiler support pragmas in code blocks (although
// fortunately clang does). Thus, only use these pragmas in the instance where
// we need it and know we can use it.
#if defined(VTKM_CLANG) && (__apple_build_version__ >= 7000072)

#define VTKM_STATIC_ASSERT(condition) \
  VTKM_THIRDPARTY_PRE_INCLUDE \
  BOOST_STATIC_ASSERT(condition) \
  VTKM_THIRDPARTY_POST_INCLUDE
#define VTKM_STATIC_ASSERT_MSG(condition, message) \
  VTKM_THIRDPARTY_PRE_INCLUDE \
  BOOST_STATIC_ASSERT_MSG(condition, message) \
  VTKM_THIRDPARTY_POST_INCLUDE

#else

#define VTKM_STATIC_ASSERT(condition) \
  BOOST_STATIC_ASSERT(condition)
#define VTKM_STATIC_ASSERT_MSG(condition, message) \
  BOOST_STATIC_ASSERT_MSG(condition, message)

#endif

#endif //vtk_m_StaticAssert_h
