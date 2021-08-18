//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ConvertNumComponentsToOffsets_h
#define vtk_m_cont_ConvertNumComponentsToOffsets_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValues.h>

namespace vtkm
{
namespace cont
{


/// \c ConvertNumComponentsToOffsets takes an array of Vec sizes (i.e. the number of components in
/// each Vec) and returns an array of offsets to a packed array of such Vecs. The resulting array
/// can be used with \c ArrayHandleGroupVecVariable.
///
/// \param numComponentsArray the input array that specifies the number of components in each group
/// Vec.
///
/// \param offsetsArray (optional) the output \c ArrayHandle, which must have a value type of \c
/// vtkm::Id. If the output \c ArrayHandle is not given, it is returned.
///
/// \param componentsArraySize (optional) a reference to a \c vtkm::Id and is filled with the expected
/// size of the component values array.
///
/// \param device (optional) specifies the device on which to run the conversion.
///
template <typename NumComponentsArrayType, typename OffsetsStorage>
VTKM_CONT void ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  using namespace vtkm::cont;
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  Algorithm::ScanExtended(device, make_ArrayHandleCast<vtkm::Id>(numComponentsArray), offsetsArray);

  componentsArraySize = ArrayGetValue(offsetsArray.GetNumberOfValues() - 1, offsetsArray);
}

template <typename NumComponentsArrayType, typename OffsetsStorage>
VTKM_CONT void ConvertNumComponentsToOffsets(
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
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::Id& componentsArraySize,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::ConvertNumComponentsToOffsets(
    numComponentsArray, offsetsArray, componentsArraySize, device);
  return offsetsArray;
}

template <typename NumComponentsArrayType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::Id dummy;
  return vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, dummy, device);
}

} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_ConvertNumComponentsToOffsets_h
