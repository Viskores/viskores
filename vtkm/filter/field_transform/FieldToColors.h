//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_FieldToColors_h
#define vtk_m_filter_field_transform_FieldToColors_h

#include <vtkm/Deprecated.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief  Convert an arbitrary field to an RGB or RGBA field
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT FieldToColors : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT
  explicit FieldToColors(const vtkm::cont::ColorTable& table = vtkm::cont::ColorTable());

  enum struct InputMode
  {
    Scalar,
    Magnitude,
    Component,
    SCALAR VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode::Scalar.") = Scalar,
    MAGNITUDE VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode::Magnitude.") = Magnitude,
    COMPONENT VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode::Component.") = Component
  };
  using FieldToColorsInputMode VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode.") = InputMode;
  VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode::Scalar.")
  static constexpr InputMode SCALAR = InputMode::Scalar;
  VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode::Magnitude.")
  static constexpr InputMode MAGNITUDE = InputMode::Magnitude;
  VTKM_DEPRECATED(1.8, "Use FieldToColors::InputMode::Component.")
  static constexpr InputMode COMPONENT = InputMode::Component;

  enum struct OutputMode
  {
    RGB,
    RGBA
  };
  using FieldToColorsOutputMode VTKM_DEPRECATED(1.8, "Use FieldToColors::OutputMode.") = OutputMode;
  VTKM_DEPRECATED(1.8, "Use FieldToColors::OutputMode::RGB.")
  static constexpr OutputMode RGB = OutputMode::RGB;
  VTKM_DEPRECATED(1.8, "Use FieldToColors::OutputMode::RGBA.")
  static constexpr OutputMode RGBA = OutputMode::RGBA;

  void SetColorTable(const vtkm::cont::ColorTable& table)
  {
    this->Table = table;
    this->ModifiedCount = -1;
  }
  const vtkm::cont::ColorTable& GetColorTable() const { return this->Table; }

  void SetMappingMode(InputMode mode) { this->InputModeType = mode; }
  void SetMappingToScalar() { this->InputModeType = InputMode::Scalar; }
  void SetMappingToMagnitude() { this->InputModeType = InputMode::Magnitude; }
  void SetMappingToComponent() { this->InputModeType = InputMode::Component; }
  InputMode GetMappingMode() const { return this->InputModeType; }
  bool IsMappingScalar() const { return this->InputModeType == InputMode::Scalar; }
  bool IsMappingMagnitude() const { return this->InputModeType == InputMode::Magnitude; }
  bool IsMappingComponent() const { return this->InputModeType == InputMode::Component; }

  void SetMappingComponent(vtkm::IdComponent comp) { this->Component = comp; }
  vtkm::IdComponent GetMappingComponent() const { return this->Component; }

  void SetOutputMode(OutputMode mode) { this->OutputModeType = mode; }
  void SetOutputToRGB() { this->OutputModeType = OutputMode::RGB; }
  void SetOutputToRGBA() { this->OutputModeType = OutputMode::RGBA; }
  OutputMode GetOutputMode() const { return this->OutputModeType; }
  bool IsOutputRGB() const { return this->OutputModeType == OutputMode::RGB; }
  bool IsOutputRGBA() const { return this->OutputModeType == OutputMode::RGBA; }


  void SetNumberOfSamplingPoints(vtkm::Int32 count);
  vtkm::Int32 GetNumberOfSamplingPoints() const { return this->SampleCount; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::cont::ColorTable Table;
  InputMode InputModeType = InputMode::Scalar;
  OutputMode OutputModeType = OutputMode::RGBA;
  vtkm::cont::ColorTableSamplesRGB SamplesRGB;
  vtkm::cont::ColorTableSamplesRGBA SamplesRGBA;
  vtkm::IdComponent Component = 0;
  vtkm::Int32 SampleCount = 256;
  vtkm::Id ModifiedCount = -1;
};
} // namespace field_transform
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::field_transform::FieldToColors.") FieldToColors
  : public vtkm::filter::field_transform::FieldToColors
{
  using field_transform::FieldToColors::FieldToColors;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_FieldToColors_h
