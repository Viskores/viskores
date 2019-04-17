//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_colorconversion_ShiftScaleToRGBA_h
#define vtk_m_worklet_colorconversion_ShiftScaleToRGBA_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/colorconversion/Conversions.h>

#include <vtkm/Math.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct ShiftScaleToRGBA : public vtkm::worklet::WorkletMapField
{
  const vtkm::Float32 Shift;
  const vtkm::Float32 Scale;
  const vtkm::Float32 Alpha;

  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ShiftScaleToRGBA(vtkm::Float32 shift, vtkm::Float32 scale, vtkm::Float32 alpha)
    : WorkletMapField()
    , Shift(shift)
    , Scale(scale)
    , Alpha(alpha)
  {
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 4> operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGBA
    vtkm::Float32 l = (static_cast<vtkm::Float32>(in) + this->Shift) * this->Scale;
    colorconversion::Clamp(l);
    const vtkm::UInt8 lc = static_cast<vtkm::UInt8>(l + 0.5);
    return vtkm::Vec<vtkm::UInt8, 4>{ lc, lc, lc, colorconversion::ColorToUChar(this->Alpha) };
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 4> operator()(const vtkm::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGBA
    vtkm::Vec<vtkm::Float32, 2> la(in);
    la = (la + vtkm::Vec<vtkm::Float32, 2>(this->Shift)) * this->Scale;
    colorconversion::Clamp(la);

    const vtkm::UInt8 lc = static_cast<vtkm::UInt8>(la[0] + 0.5f);
    return vtkm::Vec<vtkm::UInt8, 4>{
      lc, lc, lc, static_cast<vtkm::UInt8>((la[1] * this->Alpha) + 0.5f)
    };
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 4> operator()(const vtkm::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGBA
    vtkm::Vec<vtkm::Float32, 3> rgb(in);
    rgb = (rgb + vtkm::Vec<vtkm::Float32, 3>(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgb);
    return vtkm::Vec<vtkm::UInt8, 4>{ static_cast<vtkm::UInt8>(rgb[0] + 0.5f),
                                      static_cast<vtkm::UInt8>(rgb[1] + 0.5f),
                                      static_cast<vtkm::UInt8>(rgb[2] + 0.5f),
                                      colorconversion::ColorToUChar(this->Alpha) };
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 4> operator()(const vtkm::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGBA
    vtkm::Vec<vtkm::Float32, 4> rgba(in);
    rgba = (rgba + vtkm::Vec<vtkm::Float32, 4>(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgba);

    rgba[3] *= this->Alpha;
    return vtkm::Vec<vtkm::UInt8, 4>{ static_cast<vtkm::UInt8>(rgba[0] + 0.5f),
                                      static_cast<vtkm::UInt8>(rgba[1] + 0.5f),
                                      static_cast<vtkm::UInt8>(rgba[2] + 0.5f),
                                      static_cast<vtkm::UInt8>(rgba[3] + 0.5f) };
  }
};
}
}
}
#endif
