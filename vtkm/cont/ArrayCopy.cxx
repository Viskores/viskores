//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayCopyDevice.h>

namespace vtkm
{
namespace cont
{
namespace detail
{

void ArrayCopyConcreteSrc<vtkm::cont::StorageTagCounting>::CopyCountingFloat(
  vtkm::FloatDefault start,
  vtkm::FloatDefault step,
  vtkm::Id size,
  const vtkm::cont::UnknownArrayHandle& result) const
{
  if (result.IsBaseComponentType<vtkm::FloatDefault>())
  {
    auto outArray = result.ExtractComponent<vtkm::FloatDefault>(0);
    vtkm::cont::ArrayCopyDevice(vtkm::cont::make_ArrayHandleCounting(start, step, size), outArray);
  }
  else
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;
    outArray.Allocate(size);
    CopyCountingFloat(start, step, size, outArray);
    result.DeepCopyFrom(outArray);
  }
}

vtkm::cont::ArrayHandle<Id> ArrayCopyConcreteSrc<vtkm::cont::StorageTagCounting>::CopyCountingId(
  const vtkm::cont::ArrayHandleCounting<vtkm::Id>& source) const
{
  vtkm::cont::ArrayHandle<Id> destination;
  vtkm::cont::ArrayCopyDevice(source, destination);
  return destination;
}

}
}
} // namespace vtkm::cont::detail
