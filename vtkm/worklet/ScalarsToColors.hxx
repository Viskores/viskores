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

#include <vtkm/worklet/ScalarsToColors.h>

#include <vtkm/BaseComponent.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/colorconversion/ConvertToRGB.h>
#include <vtkm/worklet/colorconversion/ConvertToRGBA.h>
#include <vtkm/worklet/colorconversion/Portals.h>
#include <vtkm/worklet/colorconversion/ShiftScaleToRGB.h>
#include <vtkm/worklet/colorconversion/ShiftScaleToRGBA.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{
inline bool needShiftScale(vtkm::Float32, vtkm::Float32 shift, vtkm::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 255.0f));
}
inline bool needShiftScale(vtkm::Float64, vtkm::Float32 shift, vtkm::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 255.0f));
}
inline bool needShiftScale(vtkm::UInt8, vtkm::Float32 shift, vtkm::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 1.0f));
}

template <typename T>
inline bool needShiftScale(T, vtkm::Float32, vtkm::Float32)
{
  return true;
}
}
/// \brief Use each component to generate RGBA colors
///
template <typename T, typename S, typename Device>
void ScalarsToColors::Run(const vtkm::cont::ArrayHandle<T, S>& values,
                          vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut,
                          Device) const
{
  using namespace vtkm::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename vtkm::BaseComponent<T>::Type;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGBA, Device> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGBA, Device> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
}

/// \brief Use each component to generate RGB colors
///
template <typename T, typename S, typename Device>
void ScalarsToColors::Run(const vtkm::cont::ArrayHandle<T, S>& values,
                          vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut,
                          Device) const
{
  using namespace vtkm::worklet::colorconversion;
  using BaseT = typename vtkm::BaseComponent<T>::Type;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGB, Device> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(values, rgbOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGB, Device> dispatcher;
    dispatcher.Invoke(values, rgbOut);
  }
}

/// \brief Use magnitude of a vector to generate RGBA colors
///
template <typename T, int N, typename S, typename Device>
void ScalarsToColors::RunMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut,
                                   Device) const
{
  //magnitude is a complex situation. the default scale factor is incorrect
  //
  using namespace vtkm::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename vtkm::BaseComponent<T>::Type;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGBA, Device> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbaOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGBA, Device> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbaOut);
  }
}

/// \brief Use magnitude of a vector to generate RGB colors
///
template <typename T, int N, typename S, typename Device>
void ScalarsToColors::RunMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut,
                                   Device) const
{

  using namespace vtkm::worklet::colorconversion;
  using BaseT = typename vtkm::BaseComponent<T>::Type;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGB, Device> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGB, Device> dispatcher;
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbOut);
  }
}

/// \brief Use a single component of a vector to generate RGBA colors
///
template <typename T, int N, typename S, typename Device>
void ScalarsToColors::RunComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::IdComponent comp,
                                   vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut,
                                   Device device) const
{
  this->Run(vtkm::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
            rgbaOut,
            device);
}

/// \brief Use a single component of a vector to generate RGB colors
///
template <typename T, int N, typename S, typename Device>
void ScalarsToColors::RunComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::IdComponent comp,
                                   vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut,
                                   Device device) const
{
  this->Run(vtkm::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
            rgbOut,
            device);
}
}
}
