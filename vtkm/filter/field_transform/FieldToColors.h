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

#include <vtkm/cont/ColorTable.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief Convert an arbitrary field to an RGB or RGBA field.
///
/// This filter is useful for generating colors that could be used for rendering or
/// other purposes.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT FieldToColors : public vtkm::filter::Filter
{
public:
  VTKM_CONT
  explicit FieldToColors(const vtkm::cont::ColorTable& table = vtkm::cont::ColorTable());

  // Documentation of enumerations is behaving a little weird in Doxygen (version 1.9.7).
  // You cannot have a blank line in the documentation. Everything below it will be treated
  // as preformatted text. Also, the first line always seem to behave like `@brief` is used
  // even when it is not. It's easier to just document an associated method and copy the
  // documentation

  /// @brief Identifiers used to specify how `FieldToColors` should treat its input scalars.
  enum struct InputMode
  {
    /// @copydoc FieldToColors::SetMappingToScalar
    Scalar,
    /// @copydoc FieldToColors::SetMappingToMagnitude
    Magnitude,
    /// @copydoc FieldToColors::SetMappingToComponent
    Component,
  };

  /// @brief Identifiers used to specify what output `FieldToColors` will generate.
  enum struct OutputMode
  {
    /// @copydoc FieldToColors::SetOutputToRGB()
    RGB,
    /// @copydoc FieldToColors::SetOutputToRGBA()
    RGBA,
  };

  /// @brief Specifies the `vtkm::cont::ColorTable` object to use to map field values to colors.
  void SetColorTable(const vtkm::cont::ColorTable& table)
  {
    this->Table = table;
    this->ModifiedCount = -1;
  }
  /// @copydoc SetColorTable
  const vtkm::cont::ColorTable& GetColorTable() const { return this->Table; }

  /// @brief Specify the mapping mode.
  void SetMappingMode(InputMode mode) { this->InputModeType = mode; }
  /// @brief Treat the field as a scalar field.
  ///
  /// It is an error to provide a field of any type that cannot be directly converted
  /// to a basic floating point number (such as a vector).
  void SetMappingToScalar() { this->InputModeType = InputMode::Scalar; }
  /// @brief Map the magnitude of the field.
  ///
  /// Given a vector field, the magnitude of each field value is taken before looking it up
  /// in the color table.
  void SetMappingToMagnitude() { this->InputModeType = InputMode::Magnitude; }
  /// @brief Map a component of a vector field as if it were a scalar.
  ///
  /// Given a vector field, a particular component is looked up in the color table as if
  /// that component were in a scalar field. The component to map is selected with
  /// `SetMappingComponent()`.
  void SetMappingToComponent() { this->InputModeType = InputMode::Component; }
  /// @brief Specify the mapping mode.
  InputMode GetMappingMode() const { return this->InputModeType; }
  /// @brief Returns true if this filter is in scalar mapping mode.
  bool IsMappingScalar() const { return this->InputModeType == InputMode::Scalar; }
  /// @brief Returns true if this filter is in magnitude mapping mode.
  bool IsMappingMagnitude() const { return this->InputModeType == InputMode::Magnitude; }
  /// @brief Returns true if this filter is vector component mapping mode.
  bool IsMappingComponent() const { return this->InputModeType == InputMode::Component; }

  /// @brief Specifies the component of the vector to use in the mapping.
  ///
  /// This only has an effect if the input mapping mode is set to
  /// `FieldToColors::InputMode::Component`.
  void SetMappingComponent(vtkm::IdComponent comp) { this->Component = comp; }
  /// @copydoc SetMappingComponent
  vtkm::IdComponent GetMappingComponent() const { return this->Component; }

  /// @brief Specify the output mode.
  void SetOutputMode(OutputMode mode) { this->OutputModeType = mode; }
  /// @brief Write out RGB fixed precision color values.
  ///
  /// Output colors are represented as RGB values with each component represented by an
  /// unsigned byte. Specifically, these are `vtkm::Vec3ui_8` values.
  void SetOutputToRGB() { this->OutputModeType = OutputMode::RGB; }
  /// @brief Write out RGBA fixed precision color values.
  ///
  /// Output colors are represented as RGBA values with each component represented by an
  /// unsigned byte. Specifically, these are `vtkm::Vec4ui_8` values.
  void SetOutputToRGBA() { this->OutputModeType = OutputMode::RGBA; }
  /// @brief Specify the output mode.
  OutputMode GetOutputMode() const { return this->OutputModeType; }
  /// @brief Returns true if this filter is in RGB output mode.
  bool IsOutputRGB() const { return this->OutputModeType == OutputMode::RGB; }
  /// @brief Returns true if this filter is in RGBA output mode.
  bool IsOutputRGBA() const { return this->OutputModeType == OutputMode::RGBA; }

  /// @brief Specifies how many samples to use when looking up color values.
  ///
  /// The implementation of `FieldToColors` first builds an array of color samples to quickly
  /// look up colors for particular values. The size of this lookup array can be adjusted with
  /// this parameter. By default, an array of 256 colors is used.
  void SetNumberOfSamplingPoints(vtkm::Int32 count);
  /// @copydoc SetNumberOfSamplingPoints
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
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_FieldToColors_h
