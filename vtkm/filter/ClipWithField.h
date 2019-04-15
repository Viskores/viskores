//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithField_h
#define vtk_m_filter_ClipWithField_h

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/Clip.h>

namespace vtkm
{
namespace filter
{
/// \brief Clip a dataset using a field
///
/// Clip a dataset using a given field value. All points that are less than that
/// value are considered outside, and will be discarded. All points that are greater
/// are kept.
/// The resulting geometry will not be water tight.
class ClipWithField : public vtkm::filter::FilterDataSetWithField<ClipWithField>
{
public:
  VTKM_CONT
  ClipWithField();

  VTKM_CONT
  void SetClipValue(vtkm::Float64 value) { this->ClipValue = value; }

  VTKM_CONT
  void SetInvertClip(bool invert) { this->Invert = invert; }

  VTKM_CONT
  vtkm::Float64 GetClipValue() const { return this->ClipValue; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter.
  //This call is only valid after Execute has been called.
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::Float64 ClipValue;
  vtkm::worklet::Clip Worklet;
  bool Invert;
};

template <>
class FilterTraits<ClipWithField>
{ //currently the Clip filter only works on scalar data.
public:
  using InputFieldTypeList = TypeListTagScalarAll;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ClipWithField.hxx>

#endif // vtk_m_filter_ClipWithField_h
