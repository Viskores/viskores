//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_colorconversion_TransferFunction_h
#define vtk_m_worklet_colorconversion_TransferFunction_h

#include <vtkm/exec/ColorTable.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/colorconversion/Conversions.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct TransferFunction : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, ExecObject colorTable, FieldOut color);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T>
  VTKM_EXEC void operator()(const T& in,
                            const vtkm::exec::ColorTable& colorTable,
                            vtkm::Vec3ui_8& output) const
  {
    vtkm::Vec<float, 3> rgb = colorTable.MapThroughColorSpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
  }

  template <typename T>
  VTKM_EXEC void operator()(const T& in,
                            const vtkm::exec::ColorTable& colorTable,
                            vtkm::Vec4ui_8& output) const
  {
    vtkm::Vec<float, 3> rgb = colorTable.MapThroughColorSpace(static_cast<double>(in));
    float alpha = colorTable.MapThroughOpacitySpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
    output[3] = colorconversion::ColorToUChar(alpha);
  }

  template <typename T>
  VTKM_EXEC void operator()(const T& in,
                            const vtkm::exec::ColorTable& colorTable,
                            vtkm::Vec3f_32& output) const
  {
    output = colorTable.MapThroughColorSpace(static_cast<double>(in));
  }

  template <typename T>
  VTKM_EXEC void operator()(const T& in,
                            const vtkm::exec::ColorTable& colorTable,
                            vtkm::Vec4f_32& output) const
  {
    vtkm::Vec3f_32 rgb = colorTable.MapThroughColorSpace(static_cast<vtkm::Float64>(in));
    vtkm::Float32 alpha = colorTable.MapThroughOpacitySpace(static_cast<vtkm::Float64>(in));
    output[0] = rgb[0];
    output[1] = rgb[1];
    output[2] = rgb[2];
    output[3] = alpha;
  }
};
}
}
}
#endif
