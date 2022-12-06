//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_ClipWithField_h
#define vtk_m_filter_contour_ClipWithField_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief Clip a dataset using a field
///
/// Clip a dataset using a given field value. All points that are less than that
/// value are considered outside, and will be discarded. All points that are greater
/// are kept.
/// The resulting geometry will not be water tight.
class VTKM_FILTER_CONTOUR_EXPORT ClipWithField : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void SetClipValue(vtkm::Float64 value) { this->ClipValue = value; }

  VTKM_CONT
  void SetInvertClip(bool invert) { this->Invert = invert; }

  VTKM_CONT
  vtkm::Float64 GetClipValue() const { return this->ClipValue; }

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Float64 ClipValue = 0;
  bool Invert = false;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_ClipWithField_h
