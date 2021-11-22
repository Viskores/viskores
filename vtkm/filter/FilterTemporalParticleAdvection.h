//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_TemporalFilterParticleAdvection_h
#define vtk_m_filter_TemporalFilterParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterParticleAdvection.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.

template <class Derived>
class FilterTemporalParticleAdvection : public vtkm::filter::FilterParticleAdvection<Derived>
{
public:
  VTKM_CONT
  FilterTemporalParticleAdvection();

  VTKM_CONT
  void SetPreviousTime(vtkm::FloatDefault t) { this->PreviousTime = t; }
  VTKM_CONT
  void SetNextTime(vtkm::FloatDefault t) { this->NextTime = t; }

  VTKM_CONT
  void SetNextDataSet(const vtkm::cont::DataSet& ds)
  {
    this->NextDataSet = vtkm::cont::PartitionedDataSet(ds);
  }

  VTKM_CONT
  void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->NextDataSet = pds; }

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

protected:
  VTKM_CONT void ValidateOptions(const vtkm::cont::PartitionedDataSet& input) const;
  using vtkm::filter::FilterParticleAdvection<Derived>::ValidateOptions;

  using DSIType = vtkm::filter::particleadvection::TemporalDataSetIntegrator;
  VTKM_CONT std::vector<DSIType> CreateDataSetIntegrators(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::particleadvection::BoundsMap& boundsMap) const;

  vtkm::FloatDefault PreviousTime;
  vtkm::FloatDefault NextTime;
  vtkm::cont::PartitionedDataSet NextDataSet;

private:
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_FilterTemporalParticleAdvection_hxx
#include <vtkm/filter/FilterTemporalParticleAdvection.hxx>
#endif

#endif // vtk_m_filter_FilterTemporalParticleAdvection_h
