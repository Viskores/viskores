//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/filter/zfp/ZFPCompressor2D.h>
#include <viskores/filter/zfp/worklet/ZFP2DCompressor.h>

namespace viskores
{
namespace filter
{
namespace zfp
{
//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet ZFPCompressor2D::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::cont::CellSetStructured<2> cellSet;
  input.GetCellSet().AsCellSet(cellSet);
  viskores::Id2 pointDimensions = cellSet.GetPointDimensions();

  viskores::cont::ArrayHandle<viskores::Int64> compressed;

  viskores::worklet::ZFP2DCompressor compressor;
  using SupportedTypes = viskores::List<viskores::Int32, viskores::Float32, viskores::Float64>;
  this->GetFieldFromDataSet(input)
    .GetData()
    .CastAndCallForTypesWithFloatFallback<SupportedTypes, VISKORES_DEFAULT_STORAGE_LIST>(
      [&](const auto& concrete) {
        compressed = compressor.Compress(concrete, rate, pointDimensions);
      });

  // Note: the compressed array is set as a WholeDataSet field. It is really associated with
  // the points, but the size does not match and problems will occur if the user attempts to
  // use it as a point data set. The decompressor will place the data back as a point field.
  // (This might cause issues if cell fields are ever supported.)
  return this->CreateResultField(
    input, "compressed", viskores::cont::Field::Association::WholeDataSet, compressed);
}
} // namespace zfp
} // namespace filter
} // namespace viskores
