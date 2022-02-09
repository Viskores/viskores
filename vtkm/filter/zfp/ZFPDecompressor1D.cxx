//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/zfp/ZFPDecompressor1D.h>
#include <vtkm/filter/zfp/worklet/ZFP1DDecompress.h>

namespace vtkm
{
namespace filter
{
namespace zfp
{
//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet ZFPDecompressor1D::DoExecute(const vtkm::cont::DataSet& input)
{
  // FIXME: it looks like the compressor can compress Ints and Floats but only decompressed
  //  to Float64?
  vtkm::cont::ArrayHandle<vtkm::Int64> compressed;
  vtkm::cont::ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(input).GetData(), compressed);

  vtkm::cont::ArrayHandle<vtkm::Float64> decompressed;
  vtkm::worklet::ZFP1DDecompressor decompressor;
  decompressor.Decompress(compressed, decompressed, this->rate, compressed.GetNumberOfValues());

  return this->CreateResultFieldPoint(input, "decompressed", decompressed);
}
} // namespace zfp
} // namespace filter
} // namespace vtkm
