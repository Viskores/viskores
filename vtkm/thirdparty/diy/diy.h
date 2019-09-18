//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_thirdparty_diy_diy_h
#define vtk_m_thirdparty_diy_diy_h

#include <vtkm/thirdparty/diy/Configure.h>

#if VTKM_USE_EXTERNAL_DIY
#define VTKM_DIY_INCLUDE(header) <diy/header>
#else
#define VTKM_DIY_INCLUDE(header) <vtkmdiy/include/vtkmdiy/header>
#define diy vtkmdiy // mangle namespace diy (see below comments)
#endif

#if defined(VTKM_CLANG) || defined(VTKM_GCC)
#pragma GCC visibility push(default)
#endif

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY_INCLUDE(assigner.hpp)
#include VTKM_DIY_INCLUDE(decomposition.hpp)
#include VTKM_DIY_INCLUDE(master.hpp)
#include VTKM_DIY_INCLUDE(mpi.hpp)
#include VTKM_DIY_INCLUDE(partners/all-reduce.hpp)
#include VTKM_DIY_INCLUDE(partners/broadcast.hpp)
#include VTKM_DIY_INCLUDE(partners/swap.hpp)
#include VTKM_DIY_INCLUDE(reduce.hpp)
#include VTKM_DIY_INCLUDE(reduce-operations.hpp)
#include VTKM_DIY_INCLUDE(resolve.hpp)
#include VTKM_DIY_INCLUDE(serialization.hpp)
#undef VTKM_DIY_INCLUDE
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#if defined(VTKM_CLANG) || defined(VTKM_GCC)
#pragma GCC visibility pop
#endif

// When using an external DIY
// We need to alias the diy namespace to
// vtkmdiy so that VTK-m uses it properly
#if VTKM_USE_EXTERNAL_DIY
namespace vtkmdiy = ::diy;

#else
// The aliasing approach fails for when we
// want to us an internal version since
// the diy namespace already points to the
// external version. Instead we use macro
// replacement to make sure all diy classes
// are placed in vtkmdiy placed
#undef diy // mangle namespace diy

#endif

#endif
