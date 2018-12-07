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
#ifndef vtk_m_worklet_zfp_1d_compressor_h
#define vtk_m_worklet_zfp_1d_compressor_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/worklet/zfp/ZFPEncode1.h>
#include <vtkm/worklet/zfp/ZFPTools.h>

using ZFPWord = vtkm::UInt64;

#include <stdio.h>

namespace vtkm
{
namespace worklet
{


class ZFP1DCompressor
{
public:
  template <typename Scalar>
  vtkm::cont::ArrayHandle<vtkm::Int64> Compress(const vtkm::cont::ArrayHandle<Scalar>& data,
                                                const vtkm::Float64 requestedRate,
                                                const vtkm::Id dims)
  {
    DataDump(data, "uncompressed");
    zfp::ZFPStream stream;
    constexpr vtkm::Int32 topoDims = 1;
    vtkm::Float64 actualRate = stream.SetRate(requestedRate, topoDims, vtkm::Float64());
    //VTKM_ASSERT(
    std::cout << "ArraySize " << data.GetNumberOfValues() << "\n";
    std::cout << "Array dims " << dims << "\n";
    std::cout << "requested rate " << requestedRate << " actual rate " << actualRate << "\n";
    std::cout << "MinBits " << stream.minbits << "\n";

    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    vtkm::Id paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims % 4 != 0)
      paddedDims += 4 - dims % 4;
    constexpr vtkm::Id four = 4;
    const vtkm::Id totalBlocks = (paddedDims / four);

    std::cout << "Padded dims " << paddedDims << "\n";

    size_t outbits = detail::CalcMem1d(paddedDims, stream.minbits);
    std::cout << "Total output bits " << outbits << "\n";
    vtkm::Id outsize = outbits / sizeof(ZFPWord);
    std::cout << "Output size " << outsize << "\n";

    vtkm::cont::ArrayHandle<vtkm::Int64> output;
    // hopefully this inits/allocates the mem only on the device
    vtkm::cont::ArrayHandleConstant<vtkm::Int64> zero(0, outsize);
    vtkm::cont::Algorithm::Copy(zero, output);
    using Timer = vtkm::cont::Timer<vtkm::cont::DeviceAdapterTagSerial>;
    {
      Timer timer;
      vtkm::cont::ArrayHandleCounting<vtkm::Id> one(0, 1, 1);
      vtkm::worklet::DispatcherMapField<detail::MemTransfer> dis;
      dis.Invoke(one, data);

      vtkm::Float64 time = timer.GetElapsedTime();
      std::cout << "Copy scalars " << time << "\n";
    }

    // launch 1 thread per zfp block
    vtkm::cont::ArrayHandleCounting<vtkm::Id> blockCounter(0, 1, totalBlocks);

    Timer timer;
    vtkm::worklet::DispatcherMapField<zfp::Encode1> compressDispatcher(
      zfp::Encode1(dims, paddedDims, stream.maxbits));
    compressDispatcher.Invoke(blockCounter, data, output);

    vtkm::Float64 time = timer.GetElapsedTime();
    size_t total_bytes = data.GetNumberOfValues() * sizeof(vtkm::Float64);
    vtkm::Float64 gB = vtkm::Float64(total_bytes) / (1024. * 1024. * 1024.);
    vtkm::Float64 rate = gB / time;
    std::cout << "Compress time " << time << " sec\n";
    std::cout << "Compress rate " << rate << " GB / sec\n";
    DataDump(output, "compressed");

    return output;
  }
};
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_1d_compressor_h
