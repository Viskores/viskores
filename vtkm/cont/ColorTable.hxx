//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/worklet/DispatcherMapField.h>
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

struct map_color_table
{
  template <typename DeviceAdapter, typename ColorTable, typename... Args>
  bool operator()(DeviceAdapter device, ColorTable&& colors, Args&&... args) const
  {
    using namespace vtkm::worklet::colorconversion;
    TransferFunction transfer(colors->PrepareForExecution(device));
    vtkm::worklet::DispatcherMapField<TransferFunction, DeviceAdapter> dispatcher(transfer);
    dispatcher.Invoke(std::forward<Args>(args)...);
    return true;
  }
};

struct map_color_table_with_samples
{
  template <typename DeviceAdapter, typename... Args>
  bool operator()(DeviceAdapter,
                  const vtkm::worklet::colorconversion::LookupTable& lookupTable,
                  Args&&... args) const
  {
    using namespace vtkm::worklet::colorconversion;
    vtkm::worklet::DispatcherMapField<LookupTable, DeviceAdapter> dispatcher(lookupTable);
    dispatcher.Invoke(std::forward<Args>(args)...);
    return true;
  }
};

struct transfer_color_table_to_device
{

  template <typename DeviceAdapter, typename ColorTableInternals>
  bool operator()(DeviceAdapter device,
                  vtkm::exec::ColorTableBase* portal,
                  ColorTableInternals* internals) const
  {
    auto p1 = internals->ColorPosHandle.PrepareForInput(device);
    auto p2 = internals->ColorRGBHandle.PrepareForInput(device);
    auto p3 = internals->OpacityPosHandle.PrepareForInput(device);
    auto p4 = internals->OpacityAlphaHandle.PrepareForInput(device);
    auto p5 = internals->OpacityMidSharpHandle.PrepareForInput(device);

    //The rest of the data member on portal are set when-ever the user
    //modifies the ColorTable instance and don't need to specified here

    portal->ColorSize = static_cast<vtkm::Int32>(internals->ColorPosHandle.GetNumberOfValues());
    portal->OpacitySize = static_cast<vtkm::Int32>(internals->OpacityPosHandle.GetNumberOfValues());

    portal->ColorNodes = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p1));
    portal->RGB = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p2));
    portal->ONodes = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p3));
    portal->Alpha = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p4));
    portal->MidSharp = detail::get_ptr(vtkm::cont::ArrayPortalToIteratorBegin(p5));
    portal->Modified();
    return true;
  }
};
}

//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     const vtkm::cont::ColorTableSamplesRGBA& samples,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut) const
{
  if (samples.NumberOfSamples <= 0)
  {
    return false;
  }
  vtkm::worklet::colorconversion::LookupTable lookupTable(samples);
  return vtkm::cont::TryExecute(
    detail::map_color_table_with_samples{}, lookupTable, values, samples.Samples, rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     const vtkm::cont::ColorTableSamplesRGB& samples,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut) const
{
  if (samples.NumberOfSamples <= 0)
  {
    return false;
  }
  vtkm::worklet::colorconversion::LookupTable lookupTable(samples);
  return vtkm::cont::TryExecute(
    detail::map_color_table_with_samples{}, lookupTable, values, samples.Samples, rgbOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              const vtkm::cont::ColorTableSamplesRGBA& samples,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbaOut);
}

//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              const vtkm::cont::ColorTableSamplesRGB& samples,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut) const
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
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut) const
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
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbOut);
}

//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut) const
{
  return vtkm::cont::TryExecute(detail::map_color_table{}, this, values, rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, typename S>
bool ColorTable::Map(const vtkm::cont::ArrayHandle<T, S>& values,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut) const
{
  return vtkm::cont::TryExecute(detail::map_color_table{}, this, values, rgbOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), rgbOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::IdComponent comp,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>& rgbaOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), rgbaOut);
}
//---------------------------------------------------------------------------
template <typename T, int N, typename S>
bool ColorTable::MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                              vtkm::IdComponent comp,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>>& rgbOut) const
{
  using namespace vtkm::worklet::colorconversion;
  return this->Map(vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), rgbOut);
}
}
}
