//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_zfp_ZFPCompressor3D_h
#define vtk_m_filter_zfp_ZFPCompressor3D_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/zfp/vtkm_filter_zfp_export.h>

namespace vtkm
{
namespace filter
{
namespace zfp
{

/// \brief Compress a scalar field using ZFP.
///
/// Takes as input a 3D array and generates an
/// output of compressed data.
/// @warning
/// This filter is currently only supports 3D structured cell sets.
class VTKM_FILTER_ZFP_EXPORT ZFPCompressor3D : public vtkm::filter::Filter
{
public:
  /// @brief Specifies the rate of compression.
  void SetRate(vtkm::Float64 _rate) { rate = _rate; }
  /// @copydoc SetRate
  vtkm::Float64 GetRate() { return rate; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Float64 rate = 0;
};

} // namespace zfp
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_zfp_ZFPCompressor3D_h
