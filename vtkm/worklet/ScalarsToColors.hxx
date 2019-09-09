//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ScalarsToColors.h>

#include <vtkm/VecTraits.h>
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
template <typename T, typename S>
void ScalarsToColors::Run(const vtkm::cont::ArrayHandle<T, S>& values,
                          vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename vtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGBA> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGBA> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
}

/// \brief Use each component to generate RGB colors
///
template <typename T, typename S>
void ScalarsToColors::Run(const vtkm::cont::ArrayHandle<T, S>& values,
                          vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  using BaseT = typename vtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGB> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(values, rgbOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGB> dispatcher;
    dispatcher.Invoke(values, rgbOut);
  }
}

/// \brief Use magnitude of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  //magnitude is a complex situation. the default scale factor is incorrect
  //
  using namespace vtkm::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename vtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGBA> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbaOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGBA> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbaOut);
  }
}

/// \brief Use magnitude of a vector to generate RGB colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{

  using namespace vtkm::worklet::colorconversion;
  using BaseT = typename vtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    vtkm::worklet::DispatcherMapField<ShiftScaleToRGB> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbOut);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<ConvertToRGB> dispatcher;
    dispatcher.Invoke(
      vtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbOut);
  }
}

/// \brief Use a single component of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::IdComponent comp,
                                   vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  this->Run(vtkm::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
            rgbaOut);
}

/// \brief Use a single component of a vector to generate RGB colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                                   vtkm::IdComponent comp,
                                   vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  this->Run(vtkm::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
            rgbOut);
}
}
}
