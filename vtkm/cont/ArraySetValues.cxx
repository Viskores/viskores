//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArraySetValues.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/List.h>
#include <vtkm/TypeList.h>

void vtkm::cont::internal::ArraySetValuesImpl(const vtkm::cont::UnknownArrayHandle& ids,
                                              const vtkm::cont::UnknownArrayHandle& values,
                                              const vtkm::cont::UnknownArrayHandle& data,
                                              std::false_type)
{
  auto idArray = ids.ExtractComponent<vtkm::Id>(0, vtkm::CopyFlag::On);
  VTKM_ASSERT(ids.GetNumberOfValues() == values.GetNumberOfValues());

  bool copied = false;
  vtkm::ListForEach(
    [&](auto base) {
      using T = decltype(base);
      if (!copied && data.IsBaseComponentType<T>())
      {
        vtkm::IdComponent numComponents = data.GetNumberOfComponentsFlat();
        VTKM_ASSERT(values.GetNumberOfComponentsFlat() == numComponents);

        for (vtkm::IdComponent componentIdx = 0; componentIdx < numComponents; ++componentIdx)
        {
          auto valuesArray = values.ExtractComponent<T>(componentIdx, vtkm::CopyFlag::On);
          auto dataArray = data.ExtractComponent<T>(componentIdx, vtkm::CopyFlag::Off);
          auto permutedArray = vtkm::cont::make_ArrayHandlePermutation(idArray, dataArray);

          bool copiedComponent = false;
          copiedComponent = vtkm::cont::TryExecute([&](auto device) {
            if (dataArray.IsOnDevice(device))
            {
              vtkm::cont::DeviceAdapterAlgorithm<decltype(device)>::Copy(valuesArray,
                                                                         permutedArray);
              return true;
            }
            return false;
          });

          if (!copiedComponent)
          { // Fallback to control-side copy
            const vtkm::Id numVals = ids.GetNumberOfValues();
            auto inPortal = valuesArray.ReadPortal();
            auto outPortal = permutedArray.WritePortal();
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
    throw vtkm::cont::ErrorBadType("Unable to set values in array of type " +
                                   data.GetArrayTypeName());
  }
}
