//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_colorconversion_Conversions_h
#define vtk_m_worklet_colorconversion_Conversions_h

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

template <typename T>
VTKM_EXEC inline vtkm::UInt8 ColorToUChar(T t)
{
  return static_cast<vtkm::UInt8>(t);
}

template <>
VTKM_EXEC inline vtkm::UInt8 ColorToUChar(vtkm::Float64 t)
{
  return static_cast<vtkm::UInt8>(t * 255.0f + 0.5f);
}

template <>
VTKM_EXEC inline vtkm::UInt8 ColorToUChar(vtkm::Float32 t)
{
  return static_cast<vtkm::UInt8>(t * 255.0f + 0.5f);
}


VTKM_EXEC inline void Clamp(vtkm::Float32& val)
{
  val = vtkm::Min(255.0f, vtkm::Max(0.0f, val));
}
VTKM_EXEC inline void Clamp(vtkm::Vec2f_32& val)
{
  val[0] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[0]));
  val[1] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[1]));
}
VTKM_EXEC inline void Clamp(vtkm::Vec3f_32& val)
{
  val[0] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[0]));
  val[1] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[1]));
  val[2] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[2]));
}
VTKM_EXEC inline void Clamp(vtkm::Vec4f_32& val)
{
  val[0] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[0]));
  val[1] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[1]));
  val[2] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[2]));
  val[3] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[3]));
}
}
}
}
#endif
