//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_zfp_ZFPDecompressor2D_h
#define vtk_m_filter_zfp_ZFPDecompressor2D_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/zfp/vtkm_filter_zfp_export.h>

namespace vtkm
{
namespace filter
{
namespace zfp
{
/// \brief Compress a scalar field using ZFP

/// Takes as input a 1D array and generates on
/// output of compressed data.
/// @warning
/// This filter is currently only supports 1D volumes.
class VTKM_FILTER_ZFP_EXPORT ZFPDecompressor2D : public vtkm::filter::NewFilterField
{
public:
  void SetRate(vtkm::Float64 _rate) { rate = _rate; }
  vtkm::Float64 GetRate() { return rate; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Float64 rate = 0;
};
} // namespace zfp
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::zfp::ZFPDecompressor2D.") ZFPDecompressor2D
  : public vtkm::filter::zfp::ZFPDecompressor2D
{
  using zfp::ZFPDecompressor2D::ZFPDecompressor2D;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_zfp_ZFPDecompressor2D_h
