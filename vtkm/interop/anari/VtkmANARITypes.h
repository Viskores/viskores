//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_interop_anari_VtkmANARITypes_h
#define vtk_m_interop_anari_VtkmANARITypes_h

// vtk-m
#include <vtkm/Types.h>
// anari
#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/linalg.h>

namespace anari_cpp
{

/// Put everything from the ANARI-SDK in a single namespace to de-clutter things

using namespace ::anari;
using namespace ::anari::math;

} // namespace anari_cpp

namespace anari
{

/// These declarations let ANARI C++ bindings infer the correct `ANARIDataType`
/// enum value from VTK-m's C++ math types. This header should be included
/// before any code which needs this type inference to function.

ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec2f_32, ANARI_FLOAT32_VEC2);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec3f_32, ANARI_FLOAT32_VEC3);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec4f_32, ANARI_FLOAT32_VEC4);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec2i_32, ANARI_INT32_VEC2);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec3i_32, ANARI_INT32_VEC3);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec4i_32, ANARI_INT32_VEC4);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec2ui_32, ANARI_UINT32_VEC2);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec3ui_32, ANARI_UINT32_VEC3);
ANARI_TYPEFOR_SPECIALIZATION(vtkm::Vec4ui_32, ANARI_UINT32_VEC4);

} // namespace anari

#endif
