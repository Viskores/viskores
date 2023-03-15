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

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

/// \brief The composite vector filter combines multiple scalar fields into a single vector field.
/// Scalar fields are selected as the active input fields, and the combined vector field is set at the output.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT CompositeVectors : public vtkm::filter::FilterField
{

public:
  VTKM_CONT
  CompositeVectors() { this->SetOutputFieldName("CompositedField"); };

  VTKM_CONT
  void SetFieldNameList(
    const std::vector<std::string>& fieldNameList,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {

    vtkm::IdComponent index = 0;
    for (auto& fieldName : fieldNameList)
    {
      this->SetActiveField(index, fieldName, association);
      ++index;
    }
    this->NumberOfFields = static_cast<vtkm::IdComponent>(fieldNameList.size());
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfFields() { return this->NumberOfFields; }

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  vtkm::IdComponent NumberOfFields;
};
} // namespace field_transform
} // namespace vtkm::filter
} // namespace vtkm

#endif //vtk_m_filter_field_transform_CompositeVectors_h
