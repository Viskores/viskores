//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_colorconversion_ShiftScaleToRGB_h
#define vtk_m_worklet_colorconversion_ShiftScaleToRGB_h

#include <vtkm/worklet/colorconversion/Conversions.h>

#include <vtkm/Math.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct ShiftScaleToRGB : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ShiftScaleToRGB(vtkm::Float32 shift, vtkm::Float32 scale)
    : Shift(shift)
    , Scale(scale)
  {
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec3ui_8 operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGB
    vtkm::Float32 l = (static_cast<vtkm::Float32>(in) + this->Shift) * this->Scale;
    colorconversion::Clamp(l);
    return vtkm::Vec3ui_8{ static_cast<vtkm::UInt8>(l + 0.5f) };
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec3ui_8 operator()(const vtkm::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGB (which actually doesn't exist in vtk)
    return this->operator()(in[0]);
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec3ui_8 operator()(const vtkm::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGB
    vtkm::Vec3f_32 rgb(in);
    rgb = (rgb + vtkm::Vec3f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgb);
    return vtkm::Vec3ui_8{ static_cast<vtkm::UInt8>(rgb[0] + 0.5f),
                           static_cast<vtkm::UInt8>(rgb[1] + 0.5f),
                           static_cast<vtkm::UInt8>(rgb[2] + 0.5f) };
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec3ui_8 operator()(const vtkm::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGB
    return this->operator()(vtkm::Vec<T, 3>{ in[0], in[1], in[2] });
  }

private:
  const vtkm::Float32 Shift;
  const vtkm::Float32 Scale;
};
}
}
}
#endif
