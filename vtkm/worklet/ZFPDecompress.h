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
#ifndef vtk_m_worklet_zfp_decompressor_h
#define vtk_m_worklet_zfp_decompressor_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/zfp/ZFPDecode3.h>
#include <vtkm/worklet/zfp/ZFPTools.h>

using ZFPWord = vtkm::UInt64;

#include <stdio.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{

//size_t CalcMem3d(const vtkm::Id3 dims,
//                 const int bits_per_block)
//{
//  const size_t vals_per_block = 64;
//  const size_t size = dims[0] * dims[1] * dims[2];
//  size_t total_blocks = size / vals_per_block;
//  const size_t bits_per_word = sizeof(ZFPWord) * 8;
//  const size_t total_bits = bits_per_block * total_blocks;
//  const size_t alloc_size = total_bits / bits_per_word;
//  return alloc_size * sizeof(ZFPWord);
//}

//class MemTransfer : public vtkm::worklet::WorkletMapField
//{
//public:
//  VTKM_CONT
//  MemTransfer()
//  {
//  }
//  using ControlSignature = void(FieldIn<>, WholeArrayInOut<>);
//  using ExecutionSignature = void(_1, _2);

//  template<typename PortalType>
//  VTKM_EXEC
//  void operator()(const vtkm::Id id,
//                  PortalType& outValue) const
//  {
//    (void) id;
//    (void) outValue;
//  }
//}; //class MemTransfer

} // namespace detail


class ZFPDecompressor
{
public:
  template <typename Scalar>
  void Decompress(const vtkm::cont::ArrayHandle<vtkm::Int64>& encodedData,
                  vtkm::cont::ArrayHandle<Scalar>& output,
                  const vtkm::Float64 requestedRate,
                  vtkm::Id3 dims)
  {
    //DataDumpb(data, "uncompressed");
    zfp::ZFPStream stream;
    const vtkm::Int32 topoDims = 3;
    ;
    stream.SetRate(requestedRate, topoDims, vtkm::Float64());


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


    detail::CalcMem3d(paddedDims, stream.minbits);

    output.Allocate(dims[0] * dims[1] * dims[2]);
    // hopefully this inits/allocates the mem only on the device
    //
    //vtkm::cont::ArrayHandleConstant<vtkm::Int64> zero(0, outsize);
    //vtkm::cont::Algorithm::Copy(zero, output);
    //
    //    using Timer = vtkm::cont::Timer<vtkm::cont::DeviceAdapterTagSerial>;
    //    {
    //      Timer timer;
    //      vtkm::cont::ArrayHandleCounting<vtkm::Id> one(0,1,1);
    //      vtkm::worklet::DispatcherMapField<detail::MemTransfer> dis;
    //      dis.Invoke(one,output);
    //      dis.Invoke(one,encodedData);

    //      vtkm::Float64 time = timer.GetElapsedTime();
    //      std::cout<<"Copy scalars "<<time<<"\n";
    //    }

    // launch 1 thread per zfp block
    vtkm::cont::ArrayHandleCounting<vtkm::Id> blockCounter(0, 1, totalBlocks);

    //    Timer timer;
    vtkm::worklet::DispatcherMapField<zfp::Decode3> decompressDispatcher(
      zfp::Decode3(dims, paddedDims, stream.maxbits));
    decompressDispatcher.Invoke(blockCounter, output, encodedData);

    //    vtkm::Float64 time = timer.GetElapsedTime();
    //    size_t total_bytes =  output.GetNumberOfValues() * sizeof(vtkm::Float64);
    //    vtkm::Float64 gB = vtkm::Float64(total_bytes) / (1024. * 1024. * 1024.);
    //    vtkm::Float64 rate = gB / time;
    //    std::cout<<"Decompress time "<<time<<" sec\n";
    //    std::cout<<"Decompress rate "<<rate<<" GB / sec\n";
    //    DataDump(output, "decompressed");
  }
};
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_compressor_h
