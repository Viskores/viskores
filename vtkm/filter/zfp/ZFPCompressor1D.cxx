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

  // Note: the compressed array is set as a WholeDataSet field. It is really associated with
  // the points, but the size does not match and problems will occur if the user attempts to
  // use it as a point data set. The decompressor will place the data back as a point field.
  // (This might cause issues if cell fields are ever supported.)
  return this->CreateResultField(
    input, "compressed", vtkm::cont::Field::Association::WholeDataSet, compressed);
}
} // namespace zfp
} // namespace filter
} // namespace vtkm
