//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_OptionParser_h
#define vtk_m_cont_internal_OptionParser_h

// For warning suppression macros:
#include <vtkm/internal/ExportMacros.h>

VTKM_THIRDPARTY_PRE_INCLUDE

// Preemptively load any includes required by optionparser.h so they don't get embedded in
// our namespace.
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#endif

// We are embedding the code in optionparser.h in a VTK-m namespace so that if other code
// is using a different version the two don't get mixed up.

namespace vtkm
{
namespace cont
{
namespace internal
{

// Check to make sure that optionparser.h has not been included before. If it has, remove its
// header guard so we can include it again under our namespace.
#ifdef OPTIONPARSER_H_
#undef OPTIONPARSER_H_
#define VTK_M_REMOVED_OPTIONPARSER_HEADER_GUARD
#endif

// Include from third party.
#include <vtkmoptionparser/optionparser.h>

// Now restore the header guards as before so that other includes of (possibly different versions
// of) optionparser.h work as expected.
#ifdef VTK_M_REMOVED_OPTIONPARSER_HEADER_GUARD
// Keep header guard, but remove the macro we defined to detect that it was there.
#undef VTK_M_REMOVED_OPTIONPARSER_HEADER_GUARD
#else
// Remove the header guard for other inclusion.
#undef OPTIONPARSER_H_
#endif
}
}
} // namespace vtkm::cont::internal

VTKM_THIRDPARTY_POST_INCLUDE

#endif //vtk_m_cont_internal_OptionParser_h
