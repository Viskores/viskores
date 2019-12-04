//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_internal_ScatterBase_h
#define vtk_m_worklet_internal_ScatterBase_h

#include <vtkm/internal/ExportMacros.h>
#include <vtkm/worklet/internal/DecayHelpers.h>

namespace vtkm
{
namespace worklet
{
namespace internal
{
/// Base class for all scatter classes.
///
/// This allows VTK-m to determine when a parameter
/// is a scatter type instead of a worklet parameter.
///
struct VTKM_ALWAYS_EXPORT ScatterBase
{
};

template <typename T>
using is_scatter = std::is_base_of<ScatterBase, remove_cvref<T>>;
}
}
}
#endif
