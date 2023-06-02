//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/internal/ArrayRangeComputeUtils.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range>
ArrayRangeComputeImpl<vtkm::cont::StorageTagIndex>::operator()(
  const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagIndex>& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool vtkmNotUsed(computeFiniteRange), // assume array produces only finite values
  vtkm::cont::DeviceAdapterId device) const
{
  vtkm::Range range{};

  if (input.GetNumberOfValues() > 0)
  {
    vtkm::Id2 firstAndLast{ 0, input.GetNumberOfValues() - 1 };
    if (maskArray.GetNumberOfValues() > 0)
    {
      firstAndLast = vtkm::cont::internal::GetFirstAndLastUnmaskedIndices(maskArray, device);
    }
    if (firstAndLast[0] < firstAndLast[1])
    {
      range = vtkm::Range(firstAndLast[0], firstAndLast[1]);
    }
  }

  vtkm::cont::ArrayHandle<vtkm::Range> result;
  result.Allocate(1);
  result.WritePortal().Set(0, range);
  return result;
}

}
}
} // vtkm::cont::internal
