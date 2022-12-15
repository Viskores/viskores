//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_field_transform_LogValues_h
#define vtk_m_filter_field_transform_LogValues_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

/// \brief Adds field to a `DataSet` that gives the log values for the user specified field.
///
/// This filter use the ActiveField defined in the FilterField to store the log values.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT LogValues : public vtkm::filter::FilterField
{
public:
  enum struct LogBase
  {
    E,
    TWO,
    TEN
  };

  /// \{
  /// \brief The base value given to the log filter.
  ///
  const LogBase& GetBaseValue() const { return this->BaseValue; }
  void SetBaseValue(const LogBase& base) { this->BaseValue = base; }
  /// \}

  /// \{
  /// \brief The min value for executing the log filter.
  ///
  vtkm::FloatDefault GetMinValue() const { return this->MinValue; }
  void SetMinValue(const vtkm::FloatDefault& value) { this->MinValue = value; }
  /// \}

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  LogBase BaseValue = LogBase::E;
  vtkm::FloatDefault MinValue = std::numeric_limits<FloatDefault>::min();
};
} // namespace field_transform
} // namespace vtkm::filter
} // namespace vtkm

#endif //vtk_m_filter_field_transform_LogValues_h
