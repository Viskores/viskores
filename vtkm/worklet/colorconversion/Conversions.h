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

#include <vtkm/Math.h>

#include <cmath>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

/// Cast the provided value to a `vtkm::UInt8`. If the value is floating point,
/// it converts the range [0, 1] to [0, 255] (which is typical for how colors
/// are respectively represented in bytes and floats).
template <typename T>
VTKM_EXEC inline vtkm::UInt8 ColorToUChar(T t)
{
  return static_cast<vtkm::UInt8>(t);
}

template <>
VTKM_EXEC inline vtkm::UInt8 ColorToUChar(vtkm::Float64 t)
{
  return static_cast<vtkm::UInt8>(std::round(t * 255.0f));
}

template <>
VTKM_EXEC inline vtkm::UInt8 ColorToUChar(vtkm::Float32 t)
{
  return static_cast<vtkm::UInt8>(std::round(t * 255.0f));
}


/// Clamp the provided value to the range [0, 255].
VTKM_EXEC inline void Clamp(vtkm::Float32& val)
{
  val = vtkm::Min(255.0f, vtkm::Max(0.0f, val));
}

// Note: due to a bug in Doxygen 1.8.17, we are not using the
// vtkm::VecXT_X typedefs below.

/// Clamp the components of the provided value to the range [0, 255].
VTKM_EXEC inline void Clamp(vtkm::Vec<vtkm::Float32, 2>& val)
{
  val[0] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[0]));
  val[1] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[1]));
}

/// Clamp the components of the provided value to the range [0, 255].
VTKM_EXEC inline void Clamp(vtkm::Vec<vtkm::Float32, 3>& val)
{
  val[0] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[0]));
  val[1] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[1]));
  val[2] = vtkm::Min(255.0f, vtkm::Max(0.0f, val[2]));
}

/// Clamp the components of the provided value to the range [0, 255].
VTKM_EXEC inline void Clamp(vtkm::Vec<vtkm::Float32, 4>& val)
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
