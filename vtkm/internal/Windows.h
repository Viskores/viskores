//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_Windows_h
#define vtk_m_internal_Windows_h

#include <vtkm/internal/Configure.h>

#if defined(VTKM_WINDOWS)
// Use pragma push_macro to properly save the state of WIN32_LEAN_AND_MEAN
// and NOMINMAX that the caller of vtkm has setup

VTKM_THIRDPARTY_PRE_INCLUDE

#pragma push_macro("WIN32_LEAN_AND_MEAN")
#pragma push_macro("NOMINMAX")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// windows.h, clobbers min and max functions so we
// define NOMINMAX to fix that problem. We also include WIN32_LEAN_AND_MEAN
// to reduce the number of macros and objects windows.h imports as those also
// can cause conflicts
#include <windows.h>

#pragma pop_macro("WIN32_LEAN_AND_MEAN")
#pragma pop_macro("NOMINMAX")

VTKM_THIRDPARTY_POST_INCLUDE

#endif

#endif //vtkm_internal_Windows_h
