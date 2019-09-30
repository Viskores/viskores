//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_MaskNone_h
#define vtk_m_worklet_MaskNone_h

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/worklet/internal/MaskBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Default mask object that does not suppress anything.
///
/// \c MaskNone is a worklet mask object that does not suppress any items in the output
/// domain. This is the default mask object so that the worklet is run for every possible
/// output element.
///
struct MaskNone : public internal::MaskBase
{
  template <typename RangeType>
  VTKM_CONT RangeType GetThreadRange(RangeType outputRange) const
  {
    return outputRange;
  }

  using ThreadToOutputMapType = vtkm::cont::ArrayHandleIndex;

  VTKM_CONT ThreadToOutputMapType GetThreadToOutputMap(vtkm::Id outputRange) const
  {
    return vtkm::cont::ArrayHandleIndex(outputRange);
  }

  VTKM_CONT ThreadToOutputMapType GetThreadToOutputMap(const vtkm::Id3& outputRange) const
  {
    return this->GetThreadToOutputMap(outputRange[0] * outputRange[1] * outputRange[2]);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_MaskNone_h
