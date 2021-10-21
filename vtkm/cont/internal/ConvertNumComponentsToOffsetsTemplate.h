//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ConvertNumComponentsToOffsetsTemplate_h
#define vtk_m_cont_internal_ConvertNumComponentsToOffsetsTemplate_h


#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValues.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// @{
/// \brief Template implementation of `ConvertNumComponentsToOffsets`.
///
/// This form of the function can be used in situations where the precompiled
/// `ConvertNumComponentsToOffsets` does not include code paths for a desired
/// array.
///
template <typename NumComponentsArrayType, typename OffsetsStorage>
VTKM_CONT void ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  using namespace vtkm::cont;
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  Algorithm::ScanExtended(device, make_ArrayHandleCast<vtkm::Id>(numComponentsArray), offsetsArray);

  componentsArraySize =
    vtkm::cont::ArrayGetValue(offsetsArray.GetNumberOfValues() - 1, offsetsArray);
}

template <typename NumComponentsArrayType, typename OffsetsStorage>
VTKM_CONT void ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::Algorithm::ScanExtended(
    device, vtkm::cont::make_ArrayHandleCast<vtkm::Id>(numComponentsArray), offsetsArray);
}

template <typename NumComponentsArrayType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::internal::ConvertNumComponentsToOffsetsTemplate(
    numComponentsArray, offsetsArray, componentsArraySize, device);
  return offsetsArray;
}

template <typename NumComponentsArrayType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::internal::ConvertNumComponentsToOffsetsTemplate(
    numComponentsArray, offsetsArray, device);
  return offsetsArray;
}


/// @}

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_ConvertNumComponentsToOffsetsTemplate_h
