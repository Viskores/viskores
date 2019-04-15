//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_1d_decompressor_h
#define vtk_m_worklet_zfp_1d_decompressor_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/zfp/ZFPDecode1.h>
#include <vtkm/worklet/zfp/ZFPTools.h>

using ZFPWord = vtkm::UInt64;

#include <stdio.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{



} // namespace detail


class ZFP1DDecompressor
{
public:
  template <typename Scalar, typename StorageIn, typename StorageOut>
  void Decompress(const vtkm::cont::ArrayHandle<vtkm::Int64, StorageIn>& encodedData,
                  vtkm::cont::ArrayHandle<Scalar, StorageOut>& output,
                  const vtkm::Float64 requestedRate,
                  vtkm::Id dims)
  {
    //DataDumpb(data, "uncompressed");
    zfp::ZFPStream stream;
    constexpr vtkm::Int32 topoDims = 1;
    ;
    stream.SetRate(requestedRate, topoDims, vtkm::Float64());


    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    vtkm::Id paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims % 4 != 0)
      paddedDims += 4 - dims % 4;
    constexpr vtkm::Id four = 4;
    vtkm::Id totalBlocks = (paddedDims / four);


    zfp::detail::CalcMem1d(paddedDims, stream.minbits);

    // hopefully this inits/allocates the mem only on the device
    output.Allocate(dims);

    // launch 1 thread per zfp block
    vtkm::cont::ArrayHandleCounting<vtkm::Id> blockCounter(0, 1, totalBlocks);

    //    using Timer = vtkm::cont::Timer<vtkm::cont::DeviceAdapterTagSerial>;
    //    Timer timer;
    vtkm::worklet::DispatcherMapField<zfp::Decode1> decompressDispatcher(
      zfp::Decode1(dims, paddedDims, stream.maxbits));
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
#endif //  vtk_m_worklet_zfp_1d_decompressor_h
