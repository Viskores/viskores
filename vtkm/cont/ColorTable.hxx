//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ColorTable_hxx
#define vtk_m_cont_ColorTable_hxx

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/colorconversion/LookupTable.h>
#include <vtkm/worklet/colorconversion/Portals.h>
#include <vtkm/worklet/colorconversion/TransferFunction.h>

#include <vtkm/exec/ColorTable.h>

namespace vtkm
{
namespace cont
{

//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     const vtkm::cont::ColorTableSamplesRGBA& samples,
                     vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  if (samples.NumberOfSamples <= 0)
  {
    return false;
  }
  vtkm::worklet::colorconversion::LookupTable lookupTable(samples);
  vtkm::cont::Invoker invoke(vtkm::cont::DeviceAdapterTagAny{});
  invoke(lookupTable, values, samples.Samples, rgbaOut);
  return true;
}
//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     const vtkm::cont::ColorTableSamplesRGB& samples,
                     vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  if (samples.NumberOfSamples <= 0)
  {
    return false;
  }
  vtkm::worklet::colorconversion::LookupTable lookupTable(samples);
  vtkm::cont::Invoker invoke(vtkm::cont::DeviceAdapterTagAny{});
  invoke(lookupTable, values, samples.Samples, rgbOut);
  return true;
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              const vtkm::cont::ColorTableSamplesRGBA& samples,
                              vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbaOut);
}

//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              const vtkm::cont::ColorTableSamplesRGB& samples,
                              vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::IdComponent comp,
                              const vtkm::cont::ColorTableSamplesRGBA& samples,
                              vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::IdComponent comp,
                              const vtkm::cont::ColorTableSamplesRGB& samples,
                              vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbOut);
}

//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  vtkm::cont::Invoker invoke;
  invoke(vtkm::worklet::colorconversion::TransferFunction{}, values, *this, rgbaOut);
  return true;
}
//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  vtkm::cont::Invoker invoke;
  invoke(vtkm::worklet::colorconversion::TransferFunction{}, values, *this, rgbOut);
  return true;
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), rgbOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::IdComponent comp,
                              vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::IdComponent comp,
                              vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), rgbOut);
}

namespace
{

template <typename T>
inline vtkm::cont::ArrayHandle<T> buildSampleHandle(vtkm::Int32 numSamples,
                                                    T start,
                                                    T end,
                                                    T inc,
                                                    bool appendNanAndRangeColors)
{

  //number of samples + end + appendNanAndRangeColors
  vtkm::Int32 allocationSize = (appendNanAndRangeColors) ? numSamples + 5 : numSamples + 1;

  vtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(allocationSize);

  auto portal = handle.WritePortal();
  vtkm::Id index = 0;

  //Insert the below range first
  if (appendNanAndRangeColors)
  {
    portal.Set(index++, std::numeric_limits<T>::lowest()); //below
  }

  //add number of samples which doesn't account for the end
  T value = start;
  for (vtkm::Int32 i = 0; i < numSamples; ++i, ++index, value += inc)
  {
    portal.Set(index, value);
  }
  portal.Set(index++, end);

  if (appendNanAndRangeColors)
  {
    //push back the last value again so that when lookups near the max value
    //occur we don't need to clamp as if they are out-of-bounds they will
    //land in the extra 'end' color
    portal.Set(index++, end);
    portal.Set(index++, std::numeric_limits<T>::max()); //above
    portal.Set(index++, vtkm::Nan<T>());                //nan
  }

  return handle;
}

template <typename ColorTable, typename OutputColors>
inline bool sampleColorTable(const ColorTable* self,
                             vtkm::Int32 numSamples,
                             OutputColors& colors,
                             double tolerance,
                             bool appendNanAndRangeColors)
{
  vtkm::Range r = self->GetRange();
  //We want the samples to start at Min, and end at Max so that means
  //we want actually to interpolate numSample - 1 values. For example
  //for range 0 - 1, we want the values 0, 0.5, and 1.
  const double d_samples = static_cast<double>(numSamples - 1);
  const double d_delta = r.Length() / d_samples;

  if (r.Min > static_cast<double>(std::numeric_limits<float>::lowest()) &&
      r.Max < static_cast<double>(std::numeric_limits<float>::max()))
  {
    //we can try and see if float space has enough resolution
    const float f_samples = static_cast<float>(numSamples - 1);
    const float f_start = static_cast<float>(r.Min);
    const float f_delta = static_cast<float>(r.Length()) / f_samples;
    const float f_end = f_start + (f_delta * f_samples);

    if (vtkm::Abs(static_cast<double>(f_end) - r.Max) <= tolerance &&
        vtkm::Abs(static_cast<double>(f_delta) - d_delta) <= tolerance)
    {
      auto handle =
        buildSampleHandle((numSamples - 1), f_start, f_end, f_delta, appendNanAndRangeColors);
      return self->Map(handle, colors);
    }
  }

  //otherwise we need to use double space
  auto handle = buildSampleHandle((numSamples - 1), r.Min, r.Max, d_delta, appendNanAndRangeColors);
  return self->Map(handle, colors);
}
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(vtkm::Int32 numSamples,
                        vtkm::cont::ColorTableSamplesRGBA& samples,
                        double tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  samples.NumberOfSamples = numSamples;
  samples.SampleRange = this->GetRange();
  return sampleColorTable(this, numSamples, samples.Samples, tolerance, true);
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(vtkm::Int32 numSamples,
                        vtkm::cont::ColorTableSamplesRGB& samples,
                        double tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  samples.NumberOfSamples = numSamples;
  samples.SampleRange = this->GetRange();
  return sampleColorTable(this, numSamples, samples.Samples, tolerance, true);
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(vtkm::Int32 numSamples,
                        vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& colors,
                        double tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  return sampleColorTable(this, numSamples, colors, tolerance, false);
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(vtkm::Int32 numSamples,
                        vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& colors,
                        double tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  return sampleColorTable(this, numSamples, colors, tolerance, false);
}
}
}
#endif
