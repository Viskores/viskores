//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Mask_h
#define vtk_m_filter_Mask_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Mask.h>

namespace vtkm
{
namespace filter
{
/// \brief Subselect cells using a stride
///
/// Extract only every Nth cell where N is equal to a stride value
class Mask : public vtkm::filter::FilterDataSet<Mask>
{
public:
  VTKM_CONT
  Mask();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // Set the stride of the subsample
  VTKM_CONT
  vtkm::Id GetStride() const { return this->Stride; }
  VTKM_CONT
  void SetStride(vtkm::Id& stride) { this->Stride = stride; }

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
  vtkm::worklet::Mask Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Mask.hxx>

#endif // vtk_m_filter_Mask_h
