//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ColorTableMap_h
#define vtk_m_cont_ColorTableMap_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/ColorTableSamples.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/colorconversion/LookupTable.h>
#include <vtkm/worklet/colorconversion/Portals.h>
#include <vtkm/worklet/colorconversion/TransferFunction.h>

#include <vtkm/exec/ColorTable.h>

namespace vtkm
{
namespace cont
{

/// \brief Sample each value through an intermediate lookup/sample table to generate RGBA colors
///
/// Each value in \c values is binned based on its value in relationship to the range
/// of the color table and will use the color value at that bin from the \c samples.
/// To generate the lookup table use \c Sample .
///
/// Here is a simple example.
/// \code{.cpp}
///
/// vtkm::cont::ColorTableSamplesRGBA samples;
/// vtkm::cont::ColorTable table("black-body radiation");
/// table.Sample(256, samples);
/// vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
/// vtkm::cont::ColorTableMap(input, samples, colors);
///
/// \endcode
template <typename T, typename S>
bool ColorTableMap(const vtkm::cont::ArrayHandle<T, S>& values,
                   const vtkm::cont::ColorTableSamplesRGBA& samples,
                   vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut)
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

/// \brief Sample each value through an intermediate lookup/sample table to generate RGB colors
///
/// Each value in \c values is binned based on its value in relationship to the range
/// of the color table and will use the color value at that bin from the \c samples.
/// To generate the lookup table use \c Sample .
///
/// Here is a simple example.
/// \code{.cpp}
///
/// vtkm::cont::ColorTableSamplesRGB samples;
/// vtkm::cont::ColorTable table("black-body radiation");
/// table.Sample(256, samples);
/// vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
/// vtkm::cont::ColorTableMap(input, samples, colors);
///
/// \endcode
template <typename T, typename S>
bool ColorTableMap(const vtkm::cont::ArrayHandle<T, S>& values,
                   const vtkm::cont::ColorTableSamplesRGB& samples,
                   vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut)
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

/// \brief Use magnitude of a vector with a sample table to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            const vtkm::cont::ColorTableSamplesRGBA& samples,
                            vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbaOut);
}

/// \brief Use magnitude of a vector with a sample table to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            const vtkm::cont::ColorTableSamplesRGB& samples,
                            vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbOut);
}

/// \brief Use a single component of a vector with a sample table to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            vtkm::IdComponent comp,
                            const vtkm::cont::ColorTableSamplesRGBA& samples,
                            vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbaOut);
}

/// \brief Use a single component of a vector with a sample table to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            vtkm::IdComponent comp,
                            const vtkm::cont::ColorTableSamplesRGB& samples,
                            vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbOut);
}

/// \brief Interpolate each value through the color table to generate RGBA colors
///
/// Each value in \c values will be sampled through the entire color table
/// to determine a color.
///
/// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
template <typename T, typename S>
bool ColorTableMap(const vtkm::cont::ArrayHandle<T, S>& values,
                   const vtkm::cont::ColorTable& table,
                   vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut)
{
  vtkm::cont::Invoker invoke;
  invoke(vtkm::worklet::colorconversion::TransferFunction{}, values, table, rgbaOut);
  return true;
}

/// \brief Interpolate each value through the color table to generate RGB colors
///
/// Each value in \c values will be sampled through the entire color table
/// to determine a color.
///
/// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
template <typename T, typename S>
bool ColorTableMap(const vtkm::cont::ArrayHandle<T, S>& values,
                   const vtkm::cont::ColorTable& table,
                   vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut)
{
  vtkm::cont::Invoker invoke;
  invoke(vtkm::worklet::colorconversion::TransferFunction{}, values, table, rgbOut);
  return true;
}

/// \brief Use magnitude of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            const vtkm::cont::ColorTable& table,
                            vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), table, rgbaOut);
}

/// \brief Use magnitude of a vector to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            const vtkm::cont::ColorTable& table,
                            vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, MagnitudePortal()), table, rgbOut);
}

/// \brief Use a single component of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            vtkm::IdComponent comp,
                            const vtkm::cont::ColorTable& table,
                            vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), table, rgbaOut);
}

/// \brief Use a single component of a vector to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                            vtkm::IdComponent comp,
                            const vtkm::cont::ColorTable& table,
                            vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut)
{
  using namespace vtkm::worklet::colorconversion;
  return vtkm::cont::ColorTableMap(
    vtkm::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), table, rgbOut);
}
}
}
#endif // vtk_m_cont_ColorTableMap_h
