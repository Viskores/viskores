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
  TransferFunction(const vtkm::exec::ColorTableBase* table)
    : ColorTable(table)
  {
  }

  using ControlSignature = void(FieldIn in, FieldOut color);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const T& in, vtkm::Vec3ui_8& output) const
  {
    vtkm::Vec<float, 3> rgb = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
  }

  template <typename T>
  VTKM_EXEC void operator()(const T& in, vtkm::Vec4ui_8& output) const
  {
    vtkm::Vec<float, 3> rgb = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
    float alpha = this->ColorTable->MapThroughOpacitySpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
    output[3] = colorconversion::ColorToUChar(alpha);
  }

  template <typename T>
  VTKM_EXEC void operator()(const T& in, vtkm::Vec<float, 3>& output) const
  {
    output = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
  }

  template <typename T>
  VTKM_EXEC void operator()(const T& in, vtkm::Vec<float, 4>& output) const
  {
    vtkm::Vec<float, 3> rgb = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
    float alpha = this->ColorTable->MapThroughOpacitySpace(static_cast<double>(in));
    output[0] = rgb[0];
    output[1] = rgb[1];
    output[2] = rgb[2];
    output[3] = alpha;
  }

  const vtkm::exec::ColorTableBase* ColorTable;
};
}
}
}
#endif
