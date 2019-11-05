//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_Deprecated_h
#define vtk_m_Deprecated_h

#include <vtkm/internal/CompilerFeatures.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>

#define VTK_M_DEPRECATED_MAKE_MESSAGE(...)                                                         \
  VTKM_EXPAND(VTK_M_DEPRECATED_MAKE_MESSAGE_IMPL(__VA_ARGS__, "", vtkm::internal::NullType{}))
#define VTK_M_DEPRECATED_MAKE_MESSAGE_IMPL(version, message, ...)                                  \
  message " Deprecated in version " #version "."

/// \def VTKM_DEPRECATED(version, message)
///
/// Classes and methods are marked deprecated using the `VTKM_DEPRECATED`
/// macro. The first argument of `VTKM_DEPRECATED` should be set to the first
/// version in which the feature is deprecated. For example, if the last
/// released version of VTK-m was 1.5, and on the master branch a developer
/// wants to deprecate a class foo, then the `VTKM_DEPRECATED` release version
/// should be given as 1.6, which will be the next minor release of VTK-m. The
/// second argument of `VTKM_DEPRECATED`, which is optional but highly
/// encouraged, is a short message that should clue developers on how to update
/// their code to the new changes. For example, it could point to the
/// replacement class or method for the changed feature.
///
#define VTKM_DEPRECATED(...) VTK_M_DEPRECATED_MSG(VTK_M_DEPRECATED_MAKE_MESSAGE(__VA_ARGS__))

#endif // vtk_m_Deprecated_h
