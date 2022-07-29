//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_MapArrayPermutation_h
#define vtk_m_cont_internal_MapArrayPermutation_h

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// Used to map a permutation like that found in an ArrayHandlePermutation.
///
VTKM_CONT_EXPORT vtkm::cont::UnknownArrayHandle MapArrayPermutation(
  const vtkm::cont::UnknownArrayHandle& inputArray,
  const vtkm::cont::UnknownArrayHandle& permutation,
  vtkm::Float64 invalidValue = vtkm::Nan64());

/// Used to map a permutation array.
///
template <typename T, typename S>
vtkm::cont::UnknownArrayHandle MapArrayPermutation(
  const vtkm::cont::ArrayHandle<T,
                                vtkm::cont::StorageTagPermutation<vtkm::cont::StorageTagBasic, S>>&
    inputArray,
  vtkm::Float64 invalidValue = vtkm::Nan64())
{
  vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                     vtkm::cont::ArrayHandle<T, S>>
    input = inputArray;
  return MapArrayPermutation(input.GetValueArray(), input.GetIndexArray(), invalidValue);
}

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_MapArrayPermutation_h
