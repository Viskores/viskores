//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_resampling_Probe_h
#define vtk_m_filter_resampling_Probe_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/resampling/vtkm_filter_resampling_export.h>

namespace vtkm
{
namespace filter
{
namespace resampling
{
class VTKM_FILTER_RESAMPLING_EXPORT Probe : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void SetGeometry(const vtkm::cont::DataSet& geometry)
  {
    this->Geometry = vtkm::cont::DataSet();
    this->Geometry.CopyStructure(geometry);
  }

  VTKM_CONT
  const vtkm::cont::DataSet& GetGeometry() const { return this->Geometry; }

  VTKM_CONT void SetInvalidValue(vtkm::Float64 invalidValue) { this->InvalidValue = invalidValue; }
  VTKM_CONT vtkm::Float64 GetInvalidValue() const { return this->InvalidValue; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::cont::DataSet Geometry;

  vtkm::Float64 InvalidValue = vtkm::Nan64();
};
} // namespace resampling
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_resampling_Probe_h
