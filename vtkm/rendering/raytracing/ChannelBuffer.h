//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_rendering_raytracing_ChannelBuffer_h
#define vtkm_rendering_raytracing_ChannelBuffer_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>

#include <string>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
///
///  \brief Mananges a buffer that contains many channels per value (e.g., RGBA values).
///
///  \c The ChannelBuffer class is meant to handle a buffer of values with potentially many
///  channels. While RGBA values could be placed in a Vec<T,4>, data with a large number of
///  channels (e.g., 100+ energy bins) are better handled by a raw array. Rays can have color,
///  absorption, absorption + emission, or even track additional scalar values to support
///  standards such as Cinema. This class allows us to treat all of these different use cases
///  with the same type.
///
///  This class has methods that can be utilized by other VTK-m classes that already have a
///  a device dapter specified, and can be used by external callers where the call executes
///  on a device through the try execute mechanism.
///
///  \c Currently, the supported types are floating point to match the precision of the rays.
///

template <typename Precision>
class VTKM_RENDERING_EXPORT ChannelBuffer
{
protected:
  vtkm::Int32 NumChannels;
  vtkm::Id Size;
  std::string Name;
  friend class ChannelBufferOperations;

public:
  vtkm::cont::ArrayHandle<Precision> Buffer;

  /// Functions we want accessble outside of vtkm some of which execute
  /// on a device
  ///
  VTKM_CONT
  ChannelBuffer();

  VTKM_CONT
  ChannelBuffer(const vtkm::Int32 numChannels, const vtkm::Id size);

  VTKM_CONT
  ChannelBuffer<Precision> GetChannel(const vtkm::Int32 channel);

  ChannelBuffer<Precision> ExpandBuffer(vtkm::cont::ArrayHandle<vtkm::Id> sparseIndexes,
                                        const vtkm::Id outputSize,
                                        vtkm::cont::ArrayHandle<Precision> signature);

  ChannelBuffer<Precision> ExpandBuffer(vtkm::cont::ArrayHandle<vtkm::Id> sparseIndexes,
                                        const vtkm::Id outputSize,
                                        Precision initValue = 1.f);

  ChannelBuffer<Precision> Copy();

  void InitConst(const Precision value);
  void InitChannels(const vtkm::cont::ArrayHandle<Precision>& signature);
  void Normalize(bool invert);
  void SetName(const std::string name);
  void Resize(const vtkm::Id newSize);
  void SetNumChannels(const vtkm::Int32 numChannels);
  vtkm::Int32 GetNumChannels() const;
  vtkm::Id GetSize() const;
  vtkm::Id GetBufferLength() const;
  std::string GetName() const;
  void AddBuffer(const ChannelBuffer<Precision>& other);
  void MultiplyBuffer(const ChannelBuffer<Precision>& other);
  /// Functions that are calleble within vtkm where the device is already known
  ///
  template <typename Device>
  VTKM_CONT ChannelBuffer(const vtkm::Int32 size, const vtkm::Int32 numChannels, Device)
  {
    if (size < 1)
      throw vtkm::cont::ErrorBadValue("ChannelBuffer: Size must be greater that 0");
    if (numChannels < 1)
      throw vtkm::cont::ErrorBadValue("ChannelBuffer: NumChannels must be greater that 0");

    this->NumChannels = numChannels;
    this->Size = size;

    this->Buffer.PrepareForOutput(this->Size * this->NumChannels, Device());
  }



  template <typename Device>
  VTKM_CONT void SetNumChannels(const vtkm::Int32 numChannels, Device)
  {
    if (numChannels < 1)
    {
      std::string msg = "ChannelBuffer SetNumChannels: numBins must be greater that 0";
      throw vtkm::cont::ErrorBadValue(msg);
    }
    if (this->NumChannels == numChannels)
      return;

    this->NumChannels = numChannels;
    this->Buffer.PrepareForOutput(this->Size * this->NumChannels, Device());
  }

  template <typename Device>
  VTKM_CONT void Resize(const vtkm::Id newSize, Device device)
  {
    if (newSize < 0)
      throw vtkm::cont::ErrorBadValue("ChannelBuffer resize: Size must be greater than -1 ");
    this->Size = newSize;
    this->Buffer.PrepareForOutput(this->Size * static_cast<vtkm::Id>(NumChannels), device);
  }
};
}
}
} // namespace vtkm::rendering::raytracing

#endif
