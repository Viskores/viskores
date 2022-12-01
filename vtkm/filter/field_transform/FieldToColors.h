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
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief  Convert an arbitrary field to an RGB or RGBA field
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT FieldToColors : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  explicit FieldToColors(const vtkm::cont::ColorTable& table = vtkm::cont::ColorTable());

  enum struct InputMode
  {
    Scalar,
    Magnitude,
    Component,
  };

  enum struct OutputMode
  {
    RGB,
    RGBA
  };

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
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_FieldToColors_h
