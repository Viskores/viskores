//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_MaskIndices_h
#define vtk_m_worklet_MaskIndices_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/worklet/internal/MaskBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Mask using a given array of indices to include in the output.
///
/// \c MaskIndices is a worklet mask object that is used to select elements in the output of a
/// worklet to include in the output. This is done by providing a mask array. This array contains
/// an entry for every output to create. Any output index not included is not generated.
///
/// It is OK to give indices that are out of order, but any index must be provided at most one
/// time. It is an error to have the same index listed twice.
///
class MaskIndices : public internal::MaskBase
{
public:
  using ThreadToOutputMapType = vtkm::cont::ArrayHandle<vtkm::Id>;

  //@{
  /// \brief Construct using an index array.
  ///
  /// When you construct a \c MaskSelect with the \c IndexArray tag, you provide an array
  /// containing an index for each output to produce. It is OK to give indices that are out of
  /// order, but any index must be provided at most one time. It is an error to have the same index
  /// listed twice.
  ///
  /// Note that depending on the type of the array passed in, the index may be shallow copied
  /// or deep copied into the state of this mask object. Thus, it is a bad idea to alter the
  /// array once given to this object.
  ///
  explicit MaskIndices(
    const vtkm::cont::ArrayHandle<vtkm::Id>& indexArray,
    vtkm::cont::DeviceAdapterId vtkmNotUsed(device) = vtkm::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = indexArray;
  }

  template <typename T, typename S>
  explicit MaskIndices(const vtkm::cont::ArrayHandle<T, S>& indexArray,
                       vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
  {
    vtkm::cont::Algorithm::Copy(device, indexArray, this->ThreadToOutputMap);
  }
  //@}

  // TODO? Create a version that accepts a VariantArrayHandle. Is this needed?

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
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_MaskIndices_h
