
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/ArrayRangeComputeUtils.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>

#include <vtkm/BinaryOperators.h>

#include <limits>

namespace
{

struct UnmaskedIndicesTransform
{
  VTKM_EXEC vtkm::Id2 operator()(vtkm::Pair<vtkm::UInt8, vtkm::Id> in) const
  {
    if (in.first == 0)
    {
      return { std::numeric_limits<vtkm::Id>::max(), std::numeric_limits<vtkm::Id>::min() };
    }
    return { in.second };
  }
};

} // namespace

vtkm::Id2 vtkm::cont::internal::GetFirstAndLastUnmaskedIndices(
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  vtkm::cont::DeviceAdapterId device)
{
  vtkm::Id2 initialValue = { std::numeric_limits<vtkm::Id>::max(),
                             std::numeric_limits<vtkm::Id>::min() };
  auto maskValsAndInds = vtkm::cont::make_ArrayHandleZip(
    maskArray, vtkm::cont::ArrayHandleIndex(maskArray.GetNumberOfValues()));
  auto unmaskedIndices =
    vtkm::cont::make_ArrayHandleTransform(maskValsAndInds, UnmaskedIndicesTransform{});
  return vtkm::cont::Algorithm::Reduce(
    device, unmaskedIndices, initialValue, vtkm::MinAndMax<vtkm::Id>());
}
