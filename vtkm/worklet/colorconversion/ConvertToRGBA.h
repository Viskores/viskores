//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_colorconversion_ScalarsToColors_h
#define vtk_m_worklet_colorconversion_ScalarsToColors_h

#include <vtkm/worklet/colorconversion/Conversions.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct ConvertToRGBA : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ConvertToRGBA(vtkm::Float32 alpha)
    : Alpha(alpha)
  {
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec4ui_8 operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGBA
    const vtkm::UInt8 l = colorconversion::ColorToUChar(in);
    return vtkm::Vec<UInt8, 4>(l, l, l, colorconversion::ColorToUChar(this->Alpha));
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec4ui_8 operator()(const vtkm::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGBA
    const vtkm::UInt8 l = colorconversion::ColorToUChar(in[0]);
    const vtkm::UInt8 a = colorconversion::ColorToUChar(in[1]);
    return vtkm::Vec<UInt8, 4>(l, l, l, static_cast<vtkm::UInt8>(a * this->Alpha + 0.5f));
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec4ui_8 operator()(const vtkm::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGBA
    return vtkm::Vec<UInt8, 4>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]),
                               colorconversion::ColorToUChar(this->Alpha));
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec4ui_8 operator()(const vtkm::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGBA
    const vtkm::UInt8 a = colorconversion::ColorToUChar(in[3]);
    return vtkm::Vec<UInt8, 4>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]),
                               static_cast<vtkm::UInt8>(a * this->Alpha + 0.5f));
  }

  const vtkm::Float32 Alpha = 1.0f;
};
}
}
}
#endif
