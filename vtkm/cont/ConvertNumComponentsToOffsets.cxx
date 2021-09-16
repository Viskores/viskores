//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/cont/internal/ConvertNumComponentsToOffsetsTemplate.h>

#include <vtkm/List.h>

namespace
{

struct CallNumToOffsets
{
  template <typename BaseType>
  VTKM_CONT void operator()(BaseType,
                            const vtkm::cont::UnknownArrayHandle& numComponentsArray,
                            vtkm::cont::ArrayHandle<vtkm::Id>& offsetsArray,
                            vtkm::cont::DeviceAdapterId device,
                            bool& converted)
  {
    if (!numComponentsArray.IsBaseComponentType<BaseType>())
    {
      // Not the right type.
      return;
    }

    vtkm::cont::internal::ConvertNumComponentsToOffsetsTemplate(
      numComponentsArray.ExtractComponent<BaseType>(0, vtkm::CopyFlag::Off), // TODO: Allow copy
      offsetsArray,
      device);
    converted = true;
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{

void ConvertNumComponentsToOffsets(const vtkm::cont::UnknownArrayHandle& numComponentsArray,
                                   vtkm::cont::ArrayHandle<vtkm::Id>& offsetsArray,
                                   vtkm::Id& componentsArraySize,
                                   vtkm::cont::DeviceAdapterId device)
{
  vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, offsetsArray, device);

  componentsArraySize =
    vtkm::cont::ArrayGetValue(offsetsArray.GetNumberOfValues() - 1, offsetsArray);
}

void ConvertNumComponentsToOffsets(const vtkm::cont::UnknownArrayHandle& numComponentsArray,
                                   vtkm::cont::ArrayHandle<vtkm::Id>& offsetsArray,
                                   vtkm::cont::DeviceAdapterId device)
{
  if (numComponentsArray.GetNumberOfComponentsFlat() > 1)
  {
    throw vtkm::cont::ErrorBadType(
      "ConvertNumComponentsToOffsets only works with arrays of integers, not Vecs.");
  }

  using SupportedTypes = vtkm::List<vtkm::Int32, vtkm::Int64>;
  bool converted = false;
  vtkm::ListForEach(
    CallNumToOffsets{}, SupportedTypes{}, numComponentsArray, offsetsArray, device, converted);
  if (!converted)
  {
    internal::ThrowCastAndCallException(numComponentsArray, typeid(SupportedTypes));
  }
}

vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const vtkm::cont::UnknownArrayHandle& numComponentsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::ConvertNumComponentsToOffsets(
    numComponentsArray, offsetsArray, componentsArraySize, device);
  return offsetsArray;
}

vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const vtkm::cont::UnknownArrayHandle& numComponentsArray,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, offsetsArray, device);
  return offsetsArray;
}

} // namespace vtkm::cont
} // namespace vtkm
