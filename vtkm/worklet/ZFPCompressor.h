//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_zfp_compressor_h
#define vtk_m_worklet_zfp_compressor_h

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

using ZFPWord = vtkm::UInt64;

#define ZFP_MIN_BITS 0    /* minimum number of bits per block */
#define ZFP_MAX_BITS 4171 /* maximum number of bits per block */
#define ZFP_MAX_PREC 64   /* maximum precision supported */
#define ZFP_MIN_EXP -1074 /* minimum floating-point base-2 exponent */

namespace vtkm
{
namespace worklet
{
namespace detail
{
template <typename T>
vtkm::UInt32 MinBits(const vtkm::UInt32 bits)
{
  return bits;
}

template <>
vtkm::UInt32 MinBits<vtkm::Float32>(const vtkm::UInt32 bits)
{
  return vtkm::Max(bits, 1 + 8u);
}

template <>
vtkm::UInt32 MinBits<vtkm::Float64>(const vtkm::UInt32 bits)
{
  return vtkm::Max(bits, 1 + 11u);
}

struct Bitstream
{
};

struct ZFPStream
{
  vtkm::UInt32 minbits;
  vtkm::UInt32 maxbits;
  vtkm::UInt32 maxprec;
  vtkm::Int32 minexp;

  template <typename T>
  vtkm::Float64 SetRate(const vtkm::Float64 rate, const vtkm::Int32 dims, T vtkmNotUsed(valueType))
  {
    vtkm::UInt32 n = 1u << (2 * dims);
    vtkm::UInt32 bits = (uint)floor(n * rate + 0.5);
    bits = MinBits<T>(bits);
    //if (wra) {
    //  /* for write random access, round up to next multiple of stream word size */
    //  bits += (uint)stream_word_bits - 1;
    //  bits &= ~(stream_word_bits - 1);
    //}
    minbits = bits;
    maxbits = bits;
    maxprec = ZFP_MAX_PREC;
    minexp = ZFP_MIN_EXP;
    return (double)bits / n;
  }
};

size_t CalcMem3d(const vtkm::Id3 dims, const int bits_per_block)
{
  const size_t vals_per_block = 64;
  const size_t size = dims[0] * dims[1] * dims[2];
  size_t total_blocks = size / vals_per_block;
  const size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

//template<typename Scalar, typename PortalType> Gather3(Scalar *fblock, const PortalType &portal, )

struct Encode3 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id3 Dims;
  vtkm::Id3 PaddedDims;

public:
  Encode3(const vtkm
          : Id3 dims, const vtkm::Id3 paddedDims)
    : Dims(dims)
    , PaddedDims(paddedDims)
  {
  }
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>, WholeArrayInOut<> bitstream);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    Scalar fblock[64];
  }
};

} // namespace detail

template <typename Device>
class ZFPCompressor
{
public:
  void Compress(const vtkm::cont::ArrayHandle<vtkm::Float64>& data,
                const vtkm::Float64 requestedRate,
                const vtkm::Id3 dims)
  {
    detail::ZFPStream stream;
    const vtkm::Int32 topoDims = 3;
    ;
    vtkm::Float64 actualRate = stream.SetRate(requestedRate, topoDims, vtkm::Float64());
    //VTKM_ASSERT(
    std::cout << "ArraySize " << data.GetNumberOfValues() << "\n";
    std::cout << "Array dims " << dims << "\n";
    std::cout << "requested rate " << requestedRate << " actual rate " << actualRate << "\n";
    std::cout << "MinBits " << stream.minbits << "\n";

    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    vtkm::Id3 paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims[0] % 4 != 0)
      paddedDims[0] += 4 - dims[0] % 4;
    if (paddedDims[1] % 4 != 0)
      paddedDims[1] += 4 - dims[1] % 4;
    if (paddedDims[2] % 4 != 0)
      paddedDims[2] += 4 - dims[2] % 4;
    vtkm::Id totalBlocks = paddedDims[0] * paddedDims[1] * paddedDims[2];

    std::cout << "Padded dims " << paddedDims << "\n";

    size_t outbits = detail::CalcMem3d(dims, stream.minbits);
    std::cout << "Total output bits " << outbits << "\n";
    vtkm::Id outsize = outbits / sizeof(ZFPWord);
    std::cout << "Output size " << outsize << "\n";
    vtkm::cont::ArrayHandle<ZFPWord> output;
    output.PrepareForOutput(outsize, Device());

    vtkm::cont::ArrayHandleCounting<vtkm::Id> blockCounter(0, 1, totalBlocks);

    vtkm::worklet::DispatcherMapField<detail::Encode3, Device> compressDispatcher;
    compressDispatcher.Invoke(blockCounter, data, output);
  }
};
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_compressor_h
