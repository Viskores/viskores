//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/raytracing/ChannelBuffer.h>
#include <vtkm/rendering/raytracing/ChannelBufferOperations.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class BufferAddition : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  BufferAddition() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ValueType>
  VTKM_EXEC void operator()(const ValueType& value1, ValueType& value2) const
  {
    value2 += value1;
  }
}; //class BufferAddition

class BufferMultiply : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  BufferMultiply() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ValueType>
  VTKM_EXEC void operator()(const ValueType& value1, ValueType& value2) const
  {
    value2 *= value1;
  }
}; //class BufferMultiply

template <typename Precision>
ChannelBuffer<Precision>::ChannelBuffer()
{
  this->NumChannels = 4;
  this->Size = 0;
  this->Name = "default";
}

template <typename Precision>
ChannelBuffer<Precision>::ChannelBuffer(const vtkm::Int32 numChannels, const vtkm::Id size)
{
  if (size < 0)
    throw vtkm::cont::ErrorBadValue("ChannelBuffer: Size must be greater that -1");
  if (numChannels < 0)
    throw vtkm::cont::ErrorBadValue("ChannelBuffer: NumChannels must be greater that -1");

  this->NumChannels = numChannels;
  this->Size = size;

  Buffer.Allocate(this->Size * this->NumChannels);
}

template <typename Precision>
vtkm::Int32 ChannelBuffer<Precision>::GetNumChannels() const
{
  return this->NumChannels;
}

template <typename Precision>
vtkm::Id ChannelBuffer<Precision>::GetSize() const
{
  return this->Size;
}

template <typename Precision>
vtkm::Id ChannelBuffer<Precision>::GetBufferLength() const
{
  return this->Size * static_cast<vtkm::Id>(this->NumChannels);
}

template <typename Precision>
void ChannelBuffer<Precision>::SetName(const std::string name)
{
  this->Name = name;
}

template <typename Precision>
std::string ChannelBuffer<Precision>::GetName() const
{
  return this->Name;
}


template <typename Precision>
void ChannelBuffer<Precision>::AddBuffer(const ChannelBuffer<Precision>& other)
{
  if (this->NumChannels != other.GetNumChannels())
    throw vtkm::cont::ErrorBadValue("ChannelBuffer add: number of channels must be equal");
  if (this->Size != other.GetSize())
    throw vtkm::cont::ErrorBadValue("ChannelBuffer add: size must be equal");

  vtkm::worklet::DispatcherMapField<BufferAddition>().Invoke(other.Buffer, this->Buffer);
}

template <typename Precision>
void ChannelBuffer<Precision>::MultiplyBuffer(const ChannelBuffer<Precision>& other)
{
  if (this->NumChannels != other.GetNumChannels())
    throw vtkm::cont::ErrorBadValue("ChannelBuffer add: number of channels must be equal");
  if (this->Size != other.GetSize())
    throw vtkm::cont::ErrorBadValue("ChannelBuffer add: size must be equal");

  vtkm::worklet::DispatcherMapField<BufferMultiply>().Invoke(other.Buffer, this->Buffer);
}

template <typename Precision>
void ChannelBuffer<Precision>::Resize(const vtkm::Id newSize)
{
  if (newSize < 0)
    throw vtkm::cont::ErrorBadValue("ChannelBuffer resize: Size must be greater than -1");
  this->Size = newSize;
  this->Buffer.Allocate(this->Size * static_cast<vtkm::Id>(NumChannels));
}

class ExtractChannel : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id NumChannels; // the nnumber of channels in the buffer
  vtkm::Id ChannelNum;  // the channel to extract

public:
  VTKM_CONT
  ExtractChannel(const vtkm::Int32 numChannels, const vtkm::Int32 channel)
    : NumChannels(numChannels)
    , ChannelNum(channel)
  {
  }
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename T, typename BufferPortalType>
  VTKM_EXEC void operator()(T& outValue,
                            const BufferPortalType& inBuffer,
                            const vtkm::Id& index) const
  {
    vtkm::Id valueIndex = index * NumChannels + ChannelNum;
    BOUNDS_CHECK(inBuffer, valueIndex);
    outValue = inBuffer.Get(valueIndex);
  }
}; //class Extract Channel

template <typename Precision>
struct ExtractChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  vtkm::cont::ArrayHandle<Precision> Output;
  vtkm::Int32 Channel;

  ExtractChannelFunctor(ChannelBuffer<Precision>* self,
                        vtkm::cont::ArrayHandle<Precision> output,
                        const vtkm::Int32 channel)
    : Self(self)
    , Output(output)
    , Channel(channel)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    Output.PrepareForOutput(Self->GetSize(), device);
    vtkm::worklet::DispatcherMapField<ExtractChannel> dispatcher(
      ExtractChannel(Self->GetNumChannels(), Channel));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Output, Self->Buffer);
    return true;
  }
};

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::GetChannel(const vtkm::Int32 channel)
{
  if (channel < 0 || channel >= this->NumChannels)
    throw vtkm::cont::ErrorBadValue("ChannelBuffer: invalid channel to extract");
  ChannelBuffer<Precision> output(1, this->Size);
  output.SetName(this->Name);
  if (this->Size == 0)
  {
    return output;
  }
  ExtractChannelFunctor<Precision> functor(this, output.Buffer, channel);

  vtkm::cont::TryExecute(functor);
  return output;
}

//static
class Expand : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Int32 NumChannels;

public:
  VTKM_CONT
  Expand(const vtkm::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);
  template <typename T, typename IndexPortalType, typename BufferPortalType>
  VTKM_EXEC void operator()(const T& inValue,
                            const IndexPortalType& sparseIndexes,
                            BufferPortalType& outBuffer,
                            const vtkm::Id& index) const
  {
    vtkm::Id sparse = index / NumChannels;
    BOUNDS_CHECK(sparseIndexes, sparse);
    vtkm::Id sparseIndex = sparseIndexes.Get(sparse) * NumChannels;
    vtkm::Id outIndex = sparseIndex + index % NumChannels;
    BOUNDS_CHECK(outBuffer, outIndex);
    outBuffer.Set(outIndex, inValue);
  }
}; //class Expand

template <typename Precision>
struct ExpandFunctorSignature
{
  vtkm::cont::ArrayHandle<Precision> Input;
  vtkm::cont::ArrayHandle<vtkm::Id> SparseIndexes;
  ChannelBuffer<Precision>* Output;
  vtkm::cont::ArrayHandle<Precision> Signature;
  vtkm::Id OutputLength;
  vtkm::Int32 NumChannels;


  ExpandFunctorSignature(vtkm::cont::ArrayHandle<Precision> input,
                         vtkm::cont::ArrayHandle<vtkm::Id> sparseIndexes,
                         ChannelBuffer<Precision>* outChannelBuffer,
                         vtkm::Id outputLength,
                         vtkm::Int32 numChannels,
                         vtkm::cont::ArrayHandle<Precision> signature)
    : Input(input)
    , SparseIndexes(sparseIndexes)
    , Output(outChannelBuffer)
    , Signature(signature)
    , OutputLength(outputLength)
    , NumChannels(numChannels)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    vtkm::Id totalSize = OutputLength * static_cast<vtkm::Id>(NumChannels);
    Output->Buffer.PrepareForOutput(totalSize, device);
    ChannelBufferOperations::InitChannels(*Output, Signature, device);

    vtkm::worklet::DispatcherMapField<Expand> dispatcher((Expand(NumChannels)));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input, SparseIndexes, Output->Buffer);

    return true;
  }
};

template <typename Precision>
struct ExpandFunctor
{
  vtkm::cont::ArrayHandle<Precision> Input;
  vtkm::cont::ArrayHandle<vtkm::Id> SparseIndexes;
  ChannelBuffer<Precision>* Output;
  vtkm::Id OutputLength;
  vtkm::Int32 NumChannels;
  Precision InitVal;


  ExpandFunctor(vtkm::cont::ArrayHandle<Precision> input,
                vtkm::cont::ArrayHandle<vtkm::Id> sparseIndexes,
                ChannelBuffer<Precision>* outChannelBuffer,
                vtkm::Id outputLength,
                vtkm::Int32 numChannels,
                Precision initVal)
    : Input(input)
    , SparseIndexes(sparseIndexes)
    , Output(outChannelBuffer)
    , OutputLength(outputLength)
    , NumChannels(numChannels)
    , InitVal(initVal)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    vtkm::Id totalSize = OutputLength * static_cast<vtkm::Id>(NumChannels);
    Output->Buffer.PrepareForOutput(totalSize, device);
    ChannelBufferOperations::InitConst(*Output, InitVal, device);

    vtkm::worklet::DispatcherMapField<Expand> dispatcher((Expand(NumChannels)));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input, SparseIndexes, Output->Buffer);

    return true;
  }
};

template <typename Precision>
class NormalizeBuffer : public vtkm::worklet::WorkletMapField
{
protected:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Invert;

public:
  VTKM_CONT
  NormalizeBuffer(const Precision minScalar, const Precision maxScalar, bool invert)
    : MinScalar(minScalar)
    , Invert(invert)
  {
    if (maxScalar - minScalar == 0.)
    {
      InvDeltaScalar = MinScalar;
    }
    else
    {
      InvDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  VTKM_EXEC
  void operator()(Precision& value) const
  {
    value = (value - MinScalar) * InvDeltaScalar;
    if (Invert)
      value = 1.f - value;
  }
}; //class normalize buffer


template <typename Precision>
struct NormalizeFunctor
{
  vtkm::cont::ArrayHandle<Precision> Input;
  bool Invert;

  NormalizeFunctor(vtkm::cont::ArrayHandle<Precision> input, bool invert)
    : Input(input)
    , Invert(invert)
  {
  }

  template <typename Device>
  bool operator()(Device vtkmNotUsed(device))
  {
    auto asField = vtkm::cont::make_FieldPoint("name meaningless", this->Input);
    vtkm::Range range;
    asField.GetRange(&range);
    Precision minScalar = static_cast<Precision>(range.Min);
    Precision maxScalar = static_cast<Precision>(range.Max);
    vtkm::worklet::DispatcherMapField<NormalizeBuffer<Precision>> dispatcher(
      NormalizeBuffer<Precision>(minScalar, maxScalar, Invert));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input);

    return true;
  }
};


template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::ExpandBuffer(
  vtkm::cont::ArrayHandle<vtkm::Id> sparseIndexes,
  const vtkm::Id outputSize,
  vtkm::cont::ArrayHandle<Precision> signature)
{
  VTKM_ASSERT(this->NumChannels == signature.GetPortalConstControl().GetNumberOfValues());
  ChannelBuffer<Precision> output(this->NumChannels, outputSize);

  output.SetName(this->Name);

  ExpandFunctorSignature<Precision> functor(
    this->Buffer, sparseIndexes, &output, outputSize, this->NumChannels, signature);

  vtkm::cont::TryExecute(functor);
  return output;
}

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::ExpandBuffer(
  vtkm::cont::ArrayHandle<vtkm::Id> sparseIndexes,
  const vtkm::Id outputSize,
  Precision initValue)
{
  ChannelBuffer<Precision> output(this->NumChannels, outputSize);

  output.SetName(this->Name);

  ExpandFunctor<Precision> functor(
    this->Buffer, sparseIndexes, &output, outputSize, this->NumChannels, initValue);

  vtkm::cont::TryExecute(functor);
  return output;
}

template <typename Precision>
void ChannelBuffer<Precision>::Normalize(bool invert)
{

  auto asField = vtkm::cont::make_FieldPoint("name meaningless", this->Buffer);
  vtkm::Range range;
  asField.GetRange(&range);
  Precision minScalar = static_cast<Precision>(range.Min);
  Precision maxScalar = static_cast<Precision>(range.Max);
  vtkm::worklet::DispatcherMapField<NormalizeBuffer<Precision>> dispatcher(
    NormalizeBuffer<Precision>(minScalar, maxScalar, invert));
  dispatcher.Invoke(this->Buffer);
}

template <typename Precision>
struct ResizeChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  vtkm::Int32 NumChannels;

  ResizeChannelFunctor(ChannelBuffer<Precision>* self, vtkm::Int32 numChannels)
    : Self(self)
    , NumChannels(numChannels)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    Self->SetNumChannels(NumChannels, device);
    return true;
  }
};

template <typename Precision>
void ChannelBuffer<Precision>::InitConst(const Precision value)
{
  vtkm::cont::ArrayHandleConstant<Precision> valueHandle(value, this->GetBufferLength());
  vtkm::cont::Algorithm::Copy(valueHandle, this->Buffer);
}

template <typename Precision>
struct InitChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  const vtkm::cont::ArrayHandle<Precision>& Signature;
  InitChannelFunctor(ChannelBuffer<Precision>* self,
                     const vtkm::cont::ArrayHandle<Precision>& signature)
    : Self(self)
    , Signature(signature)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    ChannelBufferOperations::InitChannels(*Self, Signature, device);
    return true;
  }
};

template <typename Precision>
void ChannelBuffer<Precision>::InitChannels(const vtkm::cont::ArrayHandle<Precision>& signature)
{
  InitChannelFunctor<Precision> functor(this, signature);
  vtkm::cont::TryExecute(functor);
}

template <typename Precision>
void ChannelBuffer<Precision>::SetNumChannels(const vtkm::Int32 numChannels)
{
  ResizeChannelFunctor<Precision> functor(this, numChannels);
  vtkm::cont::TryExecute(functor);
}

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::Copy()
{
  ChannelBuffer res(NumChannels, Size);
  vtkm::cont::Algorithm::Copy(this->Buffer, res.Buffer);
  return res;
}

// Instantiate supported types
template class ChannelBuffer<vtkm::Float32>;
template class ChannelBuffer<vtkm::Float64>;
}
}
} // namespace vtkm::rendering::raytracing
