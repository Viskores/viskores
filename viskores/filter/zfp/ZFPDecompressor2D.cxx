//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/filter/zfp/ZFPDecompressor2D.h>
#include <viskores/filter/zfp/worklet/ZFP2DDecompress.h>

namespace viskores
{
namespace filter
{
namespace zfp
{
//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet ZFPDecompressor2D::DoExecute(
  const viskores::cont::DataSet& input)
{
  // FIXME: it looks like the compressor can compress Ints and Floats but only decompressed
  //  to Float64?
  viskores::cont::ArrayHandle<viskores::Int64> compressed;
  viskores::cont::ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(input).GetData(),
                                             compressed);

  viskores::cont::CellSetStructured<2> cellSet;
  input.GetCellSet().AsCellSet(cellSet);
  viskores::Id2 pointDimensions = cellSet.GetPointDimensions();

  viskores::cont::ArrayHandle<viskores::Float64> decompressed;
  viskores::worklet::ZFP2DDecompressor decompressor;
  decompressor.Decompress(compressed, decompressed, this->rate, pointDimensions);

  return this->CreateResultFieldPoint(input, "decompressed", decompressed);
}
} // namespace zfp
} // namespace filter
} // namespace viskores
