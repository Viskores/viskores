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
}
}
#endif
