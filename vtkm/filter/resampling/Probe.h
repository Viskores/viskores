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

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/resampling/vtkm_filter_resampling_export.h>

namespace vtkm
{
namespace worklet
{
class Probe;
}

namespace filter
{
namespace resampling
{
class VTKM_FILTER_RESAMPLING_EXPORT Probe : public vtkm::filter::NewFilterField
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

  bool DoMapField(vtkm::cont::DataSet& result,
                  const vtkm::cont::Field& field,
                  const vtkm::worklet::Probe& worklet);

  vtkm::cont::DataSet Geometry;

  vtkm::Float64 InvalidValue = vtkm::Nan64();
};
} // namespace resampling

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::resampling::Probe.") Probe
  : public vtkm::filter::resampling::Probe
{
  using resampling::Probe::Probe;
};

} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_resampling_Probe_h
