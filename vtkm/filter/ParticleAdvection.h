//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ParticleAdvection_h
#define vtk_m_filter_ParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/ParticleAdvection.h>

namespace vtkm
{
namespace filter
{
/// \brief advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.
class ParticleAdvection : public vtkm::filter::FilterDataSetWithField<ParticleAdvection>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  ParticleAdvection();

  VTKM_CONT
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  VTKM_CONT
  void SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds);

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::Id NumberOfSteps;
  vtkm::FloatDefault StepSize;
  vtkm::cont::ArrayHandle<vtkm::Particle> Seeds;
  vtkm::worklet::ParticleAdvection Worklet;
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_ParticleAdvection_hxx
#include <vtkm/filter/ParticleAdvection.hxx>
#endif

#endif // vtk_m_filter_ParticleAdvection_h
