//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_MaskSelect_h
#define vtk_m_worklet_MaskSelect_h

#include <vtkm/worklet/internal/MaskBase.h>
#include <vtkm/worklet/vtkm_worklet_export.h>

#include <vtkm/cont/VariantArrayHandle.h>

namespace vtkm
{
namespace worklet
{

/// \brief Mask using arrays to select specific elements to suppress.
///
/// \c MaskSelect is a worklet mask object that is used to select elements in the output of a
/// worklet to suppress the invocation. That is, the worklet will only be invoked for elements in
/// the output that are not masked out by the given array.
///
/// \c MaskSelect is initialized with a mask array. This array should contain a 0 for any entry
/// that should be masked and a 1 for any output that should be generated. It is an error to have
/// any value that is not a 0 or 1. This method is slower than specifying an index array.
///
class VTKM_WORKLET_EXPORT MaskSelect : public internal::MaskBase
{
  using MaskTypes =
    vtkm::List<vtkm::Int32, vtkm::Int64, vtkm::UInt32, vtkm::UInt64, vtkm::Int8, vtkm::UInt8, char>;
  using VariantArrayHandleMask = vtkm::cont::VariantArrayHandleBase<MaskTypes>;

public:
  using ThreadToOutputMapType = vtkm::cont::ArrayHandle<vtkm::Id>;

  MaskSelect(const VariantArrayHandleMask& maskArray,
             vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = this->Build(maskArray, device);
  }

  template <typename TypeList>
  MaskSelect(const vtkm::cont::VariantArrayHandleBase<TypeList>& indexArray,
             vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = this->Build(VariantArrayHandleMask(indexArray), device);
  }

  template <typename RangeType>
  vtkm::Id GetThreadRange(RangeType vtkmNotUsed(outputRange)) const
  {
    return this->ThreadToOutputMap.GetNumberOfValues();
  }

  template <typename RangeType>
  ThreadToOutputMapType GetThreadToOutputMap(RangeType vtkmNotUsed(outputRange)) const
  {
    return this->ThreadToOutputMap;
  }

private:
  ThreadToOutputMapType ThreadToOutputMap;

  VTKM_CONT ThreadToOutputMapType Build(const VariantArrayHandleMask& maskArray,
                                        vtkm::cont::DeviceAdapterId device);
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_MaskSelect_h
