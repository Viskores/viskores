//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_MaskPoints_h
#define vtk_m_filter_MaskPoints_h

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/MaskPoints.h>

namespace vtkm
{
namespace filter
{

/// \brief Subselect points using a stride
///
/// Extract only every Nth point where N is equal to a stride value
class MaskPoints : public vtkm::filter::FilterDataSet<MaskPoints>
{
public:
  VTKM_CONT
  MaskPoints();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  VTKM_CONT
  vtkm::Id GetStride() const { return this->Stride; }
  VTKM_CONT
  void SetStride(vtkm::Id stride) { this->Stride = stride; }

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::Id Stride;
  bool CompactPoints;
  vtkm::filter::CleanGrid Compactor;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/MaskPoints.hxx>

#endif // vtk_m_filter_MaskPoints_h
