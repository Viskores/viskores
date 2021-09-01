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

#include <vtkm/filter/vtkm_filter_extra_export.h>

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/MapFieldPermutation.h>

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
class VTKM_FILTER_EXTRA_EXPORT ClipWithField
  : public vtkm::filter::FilterDataSetWithField<ClipWithField>
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;

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

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    if (field.IsFieldPoint())
    {
      // If the field is a point field, then we need to do a custom interpolation of the points.
      // In this case, we need to call the superclass's MapFieldOntoOutput, which will in turn
      // call our DoMapField.
      return this->FilterDataSetWithField<ClipWithField>::MapFieldOntoOutput(result, field, policy);
    }
    else if (field.IsFieldCell())
    {
      // Use the precompiled field permutation function.
      vtkm::cont::ArrayHandle<vtkm::Id> permutation = this->Worklet.GetCellMapOutputToInput();
      return vtkm::filter::MapFieldPermutation(field, permutation, result);
    }
    else if (field.IsFieldGlobal())
    {
      result.AddField(field);
      return true;
    }
    else
    {
      return false;
    }
  }

  //Map a new field onto the resulting dataset after running the filter.
  //This call is only valid after Execute has been called.
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    // All other conditions should be handled by MapFieldOntoOutput directly.
    VTKM_ASSERT(fieldMeta.IsPointField());

    vtkm::cont::ArrayHandle<T> output;
    output = this->Worklet.ProcessPointField(input);

    //use the same meta data as the input so we get the same field name, etc.
    result.AddField(fieldMeta.AsField(output));
    return true;
  }

private:
  vtkm::Float64 ClipValue = 0;
  vtkm::worklet::Clip Worklet;
  bool Invert = false;
};

#ifndef vtkm_filter_Clip_cxx
VTKM_FILTER_EXTRA_EXPORT_EXECUTE_METHOD(ClipWithField);
#endif
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_ClipWithFieldExternInstantiations_h
#include <vtkm/filter/ClipWithFieldExternInstantiations.h>
#endif

#endif // vtk_m_filter_ClipWithField_h
