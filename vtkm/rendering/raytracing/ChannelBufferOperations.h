//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_rendering_raytracing_ChannelBuffer_Operations_h
#define vtkm_rendering_raytracing_ChannelBuffer_Operations_h

#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCast.h>

#include <vtkm/rendering/raytracing/ChannelBuffer.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class CompactBuffer : public vtkm::worklet::WorkletMapField
{
protected:
  const vtkm::Id NumChannels; // the number of channels in the buffer

public:
  VTKM_CONT
  CompactBuffer(const vtkm::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  template <typename InBufferPortalType, typename OutBufferPortalType>
  VTKM_EXEC void operator()(const vtkm::UInt8& mask,
                            const InBufferPortalType& inBuffer,
                            const vtkm::Id& offset,
                            OutBufferPortalType& outBuffer,
                            const vtkm::Id& index) const
  {
    if (mask == 0)
    {
      return;
    }
    vtkm::Id inIndex = index * NumChannels;
    vtkm::Id outIndex = offset * NumChannels;
    for (vtkm::Int32 i = 0; i < NumChannels; ++i)
    {
      BOUNDS_CHECK(inBuffer, inIndex + i);
      BOUNDS_CHECK(outBuffer, outIndex + i);
      outBuffer.Set(outIndex + i, inBuffer.Get(inIndex + i));
    }
  }
}; //class Compact

class InitBuffer : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Int32 NumChannels;

public:
  VTKM_CONT
  InitBuffer(const vtkm::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename ValueType, typename PortalType>
  VTKM_EXEC void operator()(ValueType& outValue,
                            const PortalType& source,
                            const vtkm::Id& index) const
  {
    outValue = source.Get(index % NumChannels);
  }
}; //class InitBuffer


} // namespace detail

class ChannelBufferOperations
{
public:
  template <typename Precision>
  static void Compact(ChannelBuffer<Precision>& buffer,
                      vtkm::cont::ArrayHandle<UInt8>& masks,
                      const vtkm::Id& newSize)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    offsets.Allocate(buffer.Size);
    vtkm::cont::ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::UInt8>> castedMasks(masks);
    vtkm::cont::Algorithm::ScanExclusive(castedMasks, offsets);

    vtkm::cont::ArrayHandle<Precision> compactedBuffer;
    compactedBuffer.Allocate(newSize * buffer.NumChannels);

    vtkm::worklet::DispatcherMapField<detail::CompactBuffer> dispatcher(
      detail::CompactBuffer(buffer.NumChannels));
    dispatcher.Invoke(masks, buffer.Buffer, offsets, compactedBuffer);
    buffer.Buffer = compactedBuffer;
    buffer.Size = newSize;
  }

  template <typename Device, typename Precision>
  static void InitChannels(ChannelBuffer<Precision>& buffer,
                           vtkm::cont::ArrayHandle<Precision> sourceSignature,
                           Device)
  {
    if (sourceSignature.GetNumberOfValues() != buffer.NumChannels)
    {
      std::string msg = "ChannelBuffer: number of bins in sourse signature must match NumChannels";
      throw vtkm::cont::ErrorBadValue(msg);
    }
    vtkm::worklet::DispatcherMapField<detail::InitBuffer> initBufferDispatcher(
      detail::InitBuffer(buffer.NumChannels));
    initBufferDispatcher.SetDevice(Device());
    initBufferDispatcher.Invoke(buffer.Buffer, sourceSignature);
  }

  template <typename Device, typename Precision>
  static void InitConst(ChannelBuffer<Precision>& buffer, const Precision value, Device)
  {
    vtkm::cont::ArrayHandleConstant<Precision> valueHandle(value, buffer.GetBufferLength());
    vtkm::cont::Algorithm::Copy(Device(), valueHandle, buffer.Buffer);
  }
};
}
}
} // namespace vtkm::rendering::raytracing
#endif
