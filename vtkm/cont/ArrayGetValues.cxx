//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayGetValues.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/List.h>
#include <vtkm/TypeList.h>

void vtkm::cont::internal::ArrayGetValuesImpl(const vtkm::cont::UnknownArrayHandle& ids,
                                              const vtkm::cont::UnknownArrayHandle& data,
                                              const vtkm::cont::UnknownArrayHandle& output)
{
  auto idArray = ids.ExtractComponent<vtkm::Id>(0, vtkm::CopyFlag::On);
  output.Allocate(ids.GetNumberOfValues());

  bool copied = false;
  vtkm::ListForEach(
    [&](auto base) {
      using T = decltype(base);
      if (!copied && data.IsBaseComponentType<T>())
      {
        vtkm::IdComponent numComponents = data.GetNumberOfComponentsFlat();
        VTKM_ASSERT(output.GetNumberOfComponentsFlat() == numComponents);
        for (vtkm::IdComponent componentIdx = 0; componentIdx < numComponents; ++componentIdx)
        {
          auto dataArray = data.ExtractComponent<T>(componentIdx, vtkm::CopyFlag::On);
          auto outputArray = output.ExtractComponent<T>(componentIdx, vtkm::CopyFlag::Off);
          auto permutedArray = vtkm::cont::make_ArrayHandlePermutation(idArray, dataArray);

          bool copiedComponent = false;
          if (!dataArray.IsOnHost())
          {
            copiedComponent = vtkm::cont::TryExecute([&](auto device) {
              if (dataArray.IsOnDevice(device))
              {
                vtkm::cont::DeviceAdapterAlgorithm<decltype(device)>::Copy(permutedArray,
                                                                           outputArray);
                return true;
              }
              return false;
            });
          }

          if (!copiedComponent)
          { // Fallback to a control-side copy if the device copy fails or if the device
            // is undefined or if the data were already on the host. In this case, the
            // best we can do is grab the portals and copy one at a time on the host with
            // a for loop.
            const vtkm::Id numVals = ids.GetNumberOfValues();
            auto inPortal = permutedArray.ReadPortal();
            auto outPortal = outputArray.WritePortal();
            for (vtkm::Id i = 0; i < numVals; ++i)
            {
              outPortal.Set(i, inPortal.Get(i));
            }
          }
        }

        copied = true;
      }
    },
    vtkm::TypeListBaseC{});

  if (!copied)
  {
    throw vtkm::cont::ErrorBadType("Unable to get values from array of type " +
                                   data.GetArrayTypeName());
  }
}
