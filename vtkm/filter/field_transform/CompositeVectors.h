//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_field_transform_CompositeVectors_h
#define vtk_m_filter_field_transform_CompositeVectors_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

/// @brief Combine multiple scalar fields into a single vector field.
///
/// Scalar fields are selected as the active input fields, and the combined vector
/// field is set at the output. The `SetFieldNameList()` method takes a `std::vector`
/// of field names to use as the component fields. Alternately, the `SetActiveField()`
/// method can be used to select the fields independently.
///
/// All of the input fields must be scalar values. The type of the first field
/// determines the type of the output vector field.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT CompositeVectors : public vtkm::filter::Filter
{

public:
  VTKM_CONT
  CompositeVectors() { this->SetOutputFieldName("CompositedField"); };

  /// @brief Specifies the names of the fields to use as components for the output.
  VTKM_CONT void SetFieldNameList(
    const std::vector<std::string>& fieldNameList,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any);

  /// @brief The number of fields specified as inputs.
  ///
  /// This will be the number of components in the generated field.
  VTKM_CONT vtkm::IdComponent GetNumberOfFields() const;

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace field_transform
} // namespace vtkm::filter
} // namespace vtkm

#endif //vtk_m_filter_field_transform_CompositeVectors_h
