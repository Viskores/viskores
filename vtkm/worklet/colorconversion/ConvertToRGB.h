//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_colorconversion_ConvertToRGB_h
#define vtk_m_worklet_colorconversion_ConvertToRGB_h

#include <vtkm/worklet/colorconversion/Conversions.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct ConvertToRGB : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn<> in, FieldOut<> out);
  using ExecutionSignature = _2(_1);

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 3> operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGB
    const vtkm::UInt8 la = colorconversion::ColorToUChar(in);
    return vtkm::Vec<UInt8, 3>(la, la, la);
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 3> operator()(const vtkm::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGB (which actually doesn't exist in vtk)
    return this->operator()(in[0]);
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 3> operator()(const vtkm::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGB
    return vtkm::Vec<UInt8, 3>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]));
  }

  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 3> operator()(const vtkm::Vec<vtkm::UInt8, 3>& in) const
  { //vtkScalarsToColorsRGBToRGB
    return in;
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<vtkm::UInt8, 3> operator()(const vtkm::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGB
    return vtkm::Vec<UInt8, 3>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]));
  }
};
}
}
}
#endif
