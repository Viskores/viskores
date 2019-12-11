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

/// \def VTKM_DEPRECATED_SUPPRESS_BEGIN
///
/// Begins a region of code in which warnings about using deprecated code are ignored.
/// Such suppression is usually helpful when implementing other deprecated features.
/// (You would think if one deprecated method used another deprecated method this
/// would not be a warning, but it is.)
///
/// Any use of `VTKM_DEPRECATED_SUPPRESS_BEGIN` must be paired with a
/// `VTKM_DEPRECATED_SUPPRESS_END`, which will re-enable warnings in subsequent code.
///
/// Do not use a semicolon after this macro.
///

/// \def VTKM_DEPRECATED_SUPPRESS_END
///
/// Ends a region of code in which warnings about using deprecated code are ignored.
/// Any use of `VTKM_DEPRECATED_SUPPRESS_BEGIN` must be paired with a
/// `VTKM_DEPRECATED_SUPPRESS_END`.
///
/// Do not use a semicolon after this macro.
///

// Determine whether the [[deprecated]] attribute is supported. Note that we are not
// using other older compiler features such as __attribute__((__deprecated__)) because
// they do not all support all [[deprecated]] uses (such as uses in enums). If
// [[deprecated]] is supported, then VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED will get defined.
#ifndef VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED

#if __cplusplus >= 201402L

// C++14 and better supports [[deprecated]]
#define VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED

#elif defined(VTKM_GCC)

// GCC has supported [[deprecated]] since version 5.0, but using it on enum was not
// supported until 6.0. So we have to make a special case to only use it for high
// enough revisions.
#if __GNUC__ >= 6
#define VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED
#endif // Too old GCC

#elif defined(__has_cpp_attribute)

#if __has_cpp_attribute(deprecated)
// Compiler not fully C++14 compliant, but it reports to support [[deprecated]]
#define VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED
#endif // __has_cpp_attribute(deprecated)

#elif defined(VTKM_MSVC) && _MSC_VER >= 1900

#define VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED

#endif // no known compiler support for [[deprecated]]

#endif // VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED check

// Determine how to turn deprecated warnings on and off, generally with pragmas. If
// deprecated warnings can be turned off and on, then VTK_M_DEPRECATED_SUPPRESS_SUPPORTED
// is defined and the pair VTKM_DEPRECATED_SUPPRESS_BEGIN and VTKM_DEPRECATED_SUPRESS_END
// are defined to the pragmas to disable and restore these warnings. If this support
// cannot be determined, VTK_M_DEPRECATED_SUPPRESS_SUPPORTED is _not_ define whereas
// VTKM_DEPRECATED_SUPPRESS_BEGIN and VTKM_DEPRECATED_SUPPRESS_END are defined to be
// empty.
#ifndef VTKM_DEPRECATED_SUPPRESS_SUPPORTED

#if defined(VTKM_GCC) || defined(VTKM_CLANG)

#define VTKM_DEPRECATED_SUPPRESS_SUPPORTED
#define VTKM_DEPRECATED_SUPPRESS_BEGIN                                                             \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define VTKM_DEPRECATED_SUPPRESS_END _Pragma("GCC diagnostic pop")

#elif defined(VTKM_MSVC)

#define VTKM_DEPRECATED_SUPPRESS_SUPPORTED
#define VTKM_DEPRECATED_SUPPRESS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4996))
#define VTKM_DEPRECATED_SUPPRESS_END __pragma(warning(pop))

#else

//   Other compilers probably have different pragmas for turning warnings off and on.
//   Adding more compilers to this list is fine, but the above probably capture most
//   developers and should be covered on dashboards.
#define VTKM_DEPRECATED_SUPPRESS_BEGIN
#define VTKM_DEPRECATED_SUPPRESS_END

#endif

#endif // VTKM_DEPRECATED_SUPPRESS_SUPPORTED check

#if !defined(VTKM_DEPRECATED_SUPPRESS_BEGIN) || !defined(VTKM_DEPRECATED_SUPPRESS_END)
#error VTKM_DEPRECATED_SUPPRESS macros not properly defined.
#endif

// Only actually use the [[deprecated]] attribute if the compiler supports it AND
// we know how to suppress deprecations when necessary.
#if defined(VTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED) && defined(VTKM_DEPRECATED_SUPPRESS_SUPPORTED)
#define VTKM_DEPRECATED(...) [[deprecated(VTK_M_DEPRECATED_MAKE_MESSAGE(__VA_ARGS__))]]
#else
#define VTKM_DEPRECATED(...)
#endif

#endif // vtk_m_Deprecated_h
