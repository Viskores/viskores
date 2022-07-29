//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/zfp/ZFPCompressor1D.h>
#include <vtkm/filter/zfp/worklet/ZFP1DCompressor.h>

namespace vtkm
{
namespace filter
{
namespace zfp
{
//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet ZFPCompressor1D::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);

  vtkm::cont::ArrayHandle<vtkm::Int64> compressed;
  vtkm::worklet::ZFP1DCompressor compressor;
  using SupportedTypes = vtkm::List<vtkm::Int32, vtkm::Float32, vtkm::Float64>;
  field.GetData().CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
    [&](const auto& concrete) {
      compressed = compressor.Compress(concrete, rate, field.GetNumberOfValues());
    });

  // TODO: is it really PointField or WHOLE_MESH, should we do it the same way as Histogram?
  return this->CreateResultFieldPoint(input, "compressed", compressed);
}
} // namespace zfp
} // namespace filter
} // namespace vtkm
