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
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPFunctions.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

using ZFPWord = vtkm::UInt64;

#include <stdio.h>

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
    bits = zfp::MinBits<T>(bits);
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

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Gather3(Scalar* fblock,
                              const PortalType& scalars,
                              const vtkm::Id3 dims,
                              vtkm::Id offset)
{
  // TODO: gather partial
  vtkm::Id counter = 0;
  for (vtkm::Id z = 0; z < 4; z++, offset += dims[0] * dims[1] - 4 * dims[0])
  {
    for (vtkm::Id y = 0; y < 4; y++, offset += dims[0] - 4)
    {
      for (vtkm::Id x = 0; x < 4; x++, ++offset)
      {
        fblock[counter] = scalars.Get(offset);
        counter++;
      } // x
    }   // y
  }     // z
}

struct Encode3 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id3 Dims;       // field dims
  vtkm::Id3 PaddedDims; // dims padded to a multiple of zfp block size
  vtkm::Id3 ZFPDims;    // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block
public:
  Encode3(const vtkm::Id3 dims, const vtkm::Id3 paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
    ZFPDims[2] = PaddedDims[2] / 4;
  }
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>, AtomicArrayInOut<> bitstream);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    (void)stream;
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr vtkm::Int32 BlockSize = 64;
    Scalar fblock[BlockSize];

    vtkm::Id3 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    zfpBlock[2] = blockIdx / (ZFPDims[0] * ZFPDims[1]);
    //std::cout<<"Block ID "<<blockIdx<<"\n";
    //std::cout<<"ZFP Block "<<zfpBlock<<"\n";
    vtkm::Id3 logicalStart = zfpBlock * vtkm::Id(4);
    //std::cout<<"logicalStart Start "<<logicalStart<<"\n";
    // get the offset into the field
    //vtkm::Id offset = (zfpBlock[2]*4*ZFPDims[1] + zfpBlock[1] * 4)*ZFPDims[0] * 4 + zfpBlock[0] * 4;
    vtkm::Id offset =
      (logicalStart[2] * PaddedDims[1] + logicalStart[1]) * PaddedDims[0] + logicalStart[0];
    //std::cout<<"ZFP block offset "<<offset<<"\n";
    Gather3(fblock, scalars, Dims, offset);

    //for(int i = 0; i < 64; ++i) std::cout<< fblock[i]<<" ";
    //std::cout<<"\n";
    // encode block
    vtkm::Int32 emax = zfp::MaxExponent<BlockSize, Scalar>(fblock);
    vtkm::Int32 maxprec =
      zfp::precision(emax, zfp::get_precision<Scalar>(), zfp::get_min_exp<Scalar>());
    vtkm::UInt32 e = maxprec ? emax + zfp::get_ebias<Scalar>() : 0;

    zfp::BlockWriter<BlockSize, BitstreamPortal> blockWriter(stream, MaxBits, blockIdx);
    //blockWriter.print();
    const vtkm::UInt32 ebits = zfp::get_ebits<Scalar>() + 1;
    blockWriter.write_bits(2 * e + 1, ebits);
    //std::cout<<"EBITS "<<ebits<<"\n";
    //std::cout<<"Max exponent "<<2*e+1<<" emax "<<emax<<" maxprec "<<maxprec<<" e "<<e<<"\n";
    //zfp::print_bits(2*e+1);
    //blockWriter.print();

    using Int = typename zfp::zfp_traits<Scalar>::Int;
    Int iblock[BlockSize];
    zfp::fwd_cast<Int, Scalar, BlockSize>(iblock, fblock, emax);

    zfp::encode_block<BitstreamPortal, Scalar, Int, BlockSize>(
      blockWriter, iblock, maxprec, MaxBits - ebits);
    //blockWriter.print(0);
    //blockWriter.print(1);
  }
};

template <class T>
class MemSet : public vtkm::worklet::WorkletMapField
{
  T Value;

public:
  VTKM_CONT
  MemSet(T value)
    : Value(value)
  {
  }
  using ControlSignature = void(FieldOut<>);
  using ExecutionSignature = void(_1);
  VTKM_EXEC
  void operator()(T& outValue) const { outValue = Value; }
}; //class MemSet

class MemTransfer : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  MemTransfer() {}
  using ControlSignature = void(FieldIn<>, WholeArrayInOut<>);
  using ExecutionSignature = void(_1, _2);

  template <typename PortalType>
  VTKM_EXEC void operator()(const vtkm::Id id, PortalType& outValue) const
  {
    (void)id;
    (void)outValue;
  }
}; //class MemTransfer

} // namespace detail

template <typename T>
T* GetVTKMPointer(vtkm::cont::ArrayHandle<T>& handle)
{
  typedef typename vtkm::cont::ArrayHandle<T> HandleType;
  typedef typename HandleType::template ExecutionTypes<vtkm::cont::DeviceAdapterTagSerial>::Portal
    PortalType;
  typedef typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType IteratorType;
  IteratorType iter =
    vtkm::cont::ArrayPortalToIterators<PortalType>(handle.GetPortalControl()).GetBegin();
  return &(*iter);
}

template <typename T>
void DataDump(vtkm::cont::ArrayHandle<T> handle, std::string fileName)
{

  T* ptr = GetVTKMPointer(handle);
  vtkm::Id osize = handle.GetNumberOfValues();
  FILE* fp = fopen(fileName.c_str(), "wb");
  ;
  if (fp != NULL)
  {
    fwrite(ptr, sizeof(T), osize, fp);
  }

  fclose(fp);
}

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
    const vtkm::Id four = 4;
    vtkm::Id totalBlocks =
      (paddedDims[0] / four) * (paddedDims[1] / (four) * (paddedDims[2] / four));

    std::cout << "Padded dims " << paddedDims << "\n";

    size_t outbits = detail::CalcMem3d(dims, stream.minbits);
    std::cout << "Total output bits " << outbits << "\n";
    vtkm::Id outsize = outbits / sizeof(ZFPWord);
    std::cout << "Output size " << outsize << "\n";

    vtkm::cont::ArrayHandle<vtkm::Int64> output;
    output.PrepareForOutput(outsize, Device());

    vtkm::worklet::DispatcherMapField<detail::MemSet<vtkm::Int64>> memsetDispatcher(
      detail::MemSet<vtkm::Int64>(0));
    memsetDispatcher.SetDevice(Device());
    memsetDispatcher.Invoke(output);


    {
      vtkm::cont::Timer<Device> timer;
      vtkm::cont::ArrayHandleCounting<vtkm::Id> one(0, 1, 1);
      vtkm::worklet::DispatcherMapField<detail::MemTransfer> dis;
      dis.SetDevice(Device());
      dis.Invoke(one, data);

      vtkm::Float64 time = timer.GetElapsedTime();
      std::cout << "Copy scalars " << time << "\n";
    }

    // launch 1 thread per zfp block
    vtkm::cont::ArrayHandleCounting<vtkm::Id> blockCounter(0, 1, totalBlocks);

    vtkm::cont::Timer<Device> timer;
    vtkm::worklet::DispatcherMapField<detail::Encode3> compressDispatcher(
      detail::Encode3(dims, paddedDims, stream.maxbits));
    compressDispatcher.SetDevice(Device());
    compressDispatcher.Invoke(blockCounter, data, output);
    vtkm::Float64 time = timer.GetElapsedTime();
    size_t total_bytes = data.GetNumberOfValues() * sizeof(vtkm::Float64);
    vtkm::Float64 gB = vtkm::Float64(total_bytes) / (1024. * 1024. * 1024.);
    vtkm::Float64 rate = gB / time;
    std::cout << "Compress time " << time << " sec\n";
    std::cout << "Compress rate " << rate << " GB / sec\n";
    DataDump(output, "compressed");
    DataDump(data, "uncompressed");
  }
};
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_compressor_h
