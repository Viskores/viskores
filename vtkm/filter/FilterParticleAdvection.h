//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FilterParticleAdvection_h
#define vtk_m_filter_FilterParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.

template <class Derived, typename ParticleType>
class FilterParticleAdvection : public vtkm::filter::FilterDataSetWithField<Derived>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  FilterParticleAdvection();

  VTKM_CONT
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  VTKM_CONT
  void SetSeeds(const std::vector<ParticleType>& seeds,
                vtkm::CopyFlag copyFlag = vtkm::CopyFlag::On)
  {
    this->Seeds = vtkm::cont::make_ArrayHandle(seeds, copyFlag);
  }

  VTKM_CONT
  void SetSeeds(vtkm::cont::ArrayHandle<ParticleType>& seeds) { this->Seeds = seeds; }

  VTKM_CONT
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VTKM_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet&,
                                    const vtkm::cont::Field&,
                                    vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    return false;
  }

protected:
  VTKM_CONT virtual void ValidateOptions() const;

  VTKM_CONT std::vector<vtkm::filter::particleadvection::DataSetIntegrator>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::particleadvection::BoundsMap& boundsMap) const;

  vtkm::Id NumberOfSteps;
  vtkm::FloatDefault StepSize;
  vtkm::cont::ArrayHandle<ParticleType> Seeds;
  bool UseThreadedAlgorithm;

private:
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_FilterParticleAdvection_hxx
#include <vtkm/filter/FilterParticleAdvection.hxx>
#endif

#endif // vtk_m_filter_FilterParticleAdvection_h
