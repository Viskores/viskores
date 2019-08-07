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
namespace detail
{

template <typename T>
inline T* get_ptr(T* t)
{
  return t;
}
#if defined(VTKM_MSVC)
//ArrayPortalToIteratorBegin could be returning a checked_array_iterator so
//we need to grab the underlying pointer
template <typename T>
inline T* get_ptr(stdext::checked_array_iterator<T*> t)
{
  return t.base();
}
#endif

struct transfer_color_table_to_device
{

  template <typename DeviceAdapter>
  inline bool operator()(DeviceAdapter device, vtkm::cont::ColorTable::TransferState&& state) const
  {
    auto p1 = state.ColorPosHandle.PrepareForInput(device);
    auto p2 = state.ColorRGBHandle.PrepareForInput(device);
    auto p3 = state.OpacityPosHandle.PrepareForInput(device);
    auto p4 = state.OpacityAlphaHandle.PrepareForInput(device);
    auto p5 = state.OpacityMidSharpHandle.PrepareForInput(device);

    //The rest of the data member on portal are set when-ever the user
    //modifies the ColorTable instance and don't need to specified here

    state.Portal->ColorSize = static_cast<vtkm::Int32>(state.ColorPosHandle.GetNumberOfValues());
    state.Portal->OpacitySize =
      static_cast<vtkm::Int32>(state.OpacityPosHandle.GetNumberOfValues());

    state.Portal->ColorNodes = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p1));
    state.Portal->RGB = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p2));
    state.Portal->ONodes = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p3));
    state.Portal->Alpha = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p4));
    state.Portal->MidSharp = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p5));
    state.Portal->Modified();
    return true;
  }
};

struct map_color_table
{
  template <typename DeviceAdapter, typename ColorTable, typename... Args>
  inline bool operator()(DeviceAdapter device, ColorTable&& colors, Args&&... args) const
  {
    vtkm::worklet::colorconversion::TransferFunction transfer(colors->PrepareForExecution(device));
    vtkm::cont::Invoker invoke(device);
    invoke(transfer, std::forward<Args>(args)...);
    return true;
  }
};
}

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
  return vtkm::cont::TryExecute(detail::map_color_table{}, this, values, rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const
{
  return vtkm::cont::TryExecute(detail::map_color_table{}, this, values, rgbOut);
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

  auto portal = handle.GetPortalControl();
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

//---------------------------------------------------------------------------
const vtkm::exec::ColorTableBase* ColorTable::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  //Build the ColorTable instance that is needed for execution
  if (this->NeedToCreateExecutionColorTable())
  {
    auto space = this->GetColorSpace();
    auto hostPortal = this->GetControlRepresentation();
    //Remove any existing host and execution data. The allocation of the
    //virtual object handle needs to occur in the .hxx so that it happens
    //in the same library as the user and will be a valid virtual object
    using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::ColorTableBase>;
    switch (space)
    {
      case vtkm::cont::ColorSpace::RGB:
      {
        this->UpdateExecutionColorTable(
          new HandleType(static_cast<vtkm::exec::ColorTableRGB*>(hostPortal), false));
        break;
      }
      case vtkm::cont::ColorSpace::HSV:
      {
        this->UpdateExecutionColorTable(
          new HandleType(static_cast<vtkm::exec::ColorTableHSV*>(hostPortal), false));
        break;
      }
      case vtkm::cont::ColorSpace::HSV_WRAP:
      {
        this->UpdateExecutionColorTable(
          new HandleType(static_cast<vtkm::exec::ColorTableHSVWrap*>(hostPortal), false));
        break;
      }
      case vtkm::cont::ColorSpace::LAB:
      {
        this->UpdateExecutionColorTable(
          new HandleType(static_cast<vtkm::exec::ColorTableLab*>(hostPortal), false));
        break;
      }
      case vtkm::cont::ColorSpace::DIVERGING:
      {
        this->UpdateExecutionColorTable(
          new HandleType(static_cast<vtkm::exec::ColorTableDiverging*>(hostPortal), false));
        break;
      }
    }
  }


  //transfer ColorTable and all related data
  auto&& info = this->GetExecutionDataForTransfer();
  if (info.NeedsTransfer)
  {
    bool transfered = vtkm::cont::TryExecuteOnDevice(
      device, detail::transfer_color_table_to_device{}, std::move(info));
    if (!transfered)
    {
      throwFailedRuntimeDeviceTransfer("ColorTable", device);
    }
  }
  return this->GetExecutionHandle()->PrepareForExecution(device);
}
}
}
#endif
