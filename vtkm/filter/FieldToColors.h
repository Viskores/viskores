//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FieldToColors_h
#define vtk_m_filter_FieldToColors_h

#include <vtkm/cont/ColorTable.h>
#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{

/// \brief  Convert an arbitrary field to an RGB or RGBA field
///
class FieldToColors : public vtkm::filter::FilterField<FieldToColors>
{
public:
  VTKM_CONT
  FieldToColors(const vtkm::cont::ColorTable& table = vtkm::cont::ColorTable());

  enum FieldToColorsInputMode
  {
    SCALAR,
    MAGNITUDE,
    COMPONENT
  };

  enum FieldToColorsOutputMode
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

  void SetMappingMode(FieldToColorsInputMode mode) { this->InputMode = mode; }
  void SetMappingToScalar() { this->InputMode = FieldToColorsInputMode::SCALAR; }
  void SetMappingToMagnitude() { this->InputMode = FieldToColorsInputMode::MAGNITUDE; }
  void SetMappingToComponent() { this->InputMode = FieldToColorsInputMode::COMPONENT; }
  FieldToColorsInputMode GetMappingMode() const { return this->InputMode; }
  bool IsMappingScalar() const { return this->InputMode == FieldToColorsInputMode::SCALAR; }
  bool IsMappingMagnitude() const { return this->InputMode == FieldToColorsInputMode::MAGNITUDE; }
  bool IsMappingComponent() const { return this->InputMode == FieldToColorsInputMode::COMPONENT; }

  void SetMappingComponent(vtkm::IdComponent comp) { this->Component = comp; }
  vtkm::IdComponent GetMappingComponent() const { return this->Component; }

  void SetOutputMode(FieldToColorsOutputMode mode) { this->OutputMode = mode; }
  void SetOutputToRGB() { this->OutputMode = FieldToColorsOutputMode::RGB; }
  void SetOutputToRGBA() { this->OutputMode = FieldToColorsOutputMode::RGBA; }
  FieldToColorsOutputMode GetOutputMode() const { return this->OutputMode; }
  bool IsOutputRGB() const { return this->OutputMode == FieldToColorsOutputMode::RGB; }
  bool IsOutputRGBA() const { return this->OutputMode == FieldToColorsOutputMode::RGBA; }


  void SetNumberOfSamplingPoints(vtkm::Int32 count);
  vtkm::Int32 GetNumberOfSamplingPoints() const { return this->SampleCount; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::cont::ColorTable Table;
  FieldToColorsInputMode InputMode;
  FieldToColorsOutputMode OutputMode;
  vtkm::cont::ColorTableSamplesRGB SamplesRGB;
  vtkm::cont::ColorTableSamplesRGBA SamplesRGBA;
  vtkm::IdComponent Component;
  vtkm::Int32 SampleCount;
  vtkm::Id ModifiedCount;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/FieldToColors.hxx>

#endif // vtk_m_filter_FieldToColors_h
