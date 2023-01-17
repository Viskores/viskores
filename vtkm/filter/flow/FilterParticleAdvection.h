//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_FilterParticleAdvection_h
#define vtk_m_filter_flow_FilterParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.

class VTKM_FILTER_FLOW_EXPORT FilterParticleAdvection : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  bool CanThread() const override { return false; }

  VTKM_CONT
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  template <typename ParticleType>
  VTKM_CONT void SetSeeds(vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->Seeds = seeds;
  }

  template <typename ParticleType>
  VTKM_CONT void SetSeeds(const std::vector<ParticleType>& seeds,
                          vtkm::CopyFlag copyFlag = vtkm::CopyFlag::On)
  {
    this->Seeds = vtkm::cont::make_ArrayHandle(seeds, copyFlag);
  }

  VTKM_CONT
  void SetSolverRK4() { this->SolverType = vtkm::filter::flow::IntegrationSolverType::RK4_TYPE; }

  VTKM_CONT
  void SetSolverEuler()
  {
    this->SolverType = vtkm::filter::flow::IntegrationSolverType::EULER_TYPE;
  }

  VTKM_CONT
  void SetVectorFieldType(vtkm::filter::flow::VectorFieldType vecFieldType)
  {
    this->VecFieldType = vecFieldType;
  }

  ///@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  VTKM_CONT void SetEField(const std::string& name) { this->SetActiveField(0, name); }

  VTKM_CONT void SetBField(const std::string& name) { this->SetActiveField(1, name); }

  VTKM_CONT std::string GetEField() const { return this->GetActiveFieldName(0); }

  VTKM_CONT std::string GetBField() const { return this->GetActiveFieldName(1); }

  VTKM_CONT
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VTKM_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

protected:
  VTKM_CONT virtual void ValidateOptions() const;

  VTKM_CONT virtual vtkm::filter::flow::FlowResultType GetResultType() const = 0;

  vtkm::Id NumberOfSteps = 0;
  vtkm::cont::UnknownArrayHandle Seeds;
  vtkm::filter::flow::IntegrationSolverType SolverType =
    vtkm::filter::flow::IntegrationSolverType::RK4_TYPE;
  vtkm::FloatDefault StepSize = 0;
  bool UseThreadedAlgorithm = false;
  vtkm::filter::flow::VectorFieldType VecFieldType =
    vtkm::filter::flow::VectorFieldType::VELOCITY_FIELD_TYPE;

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_FilterParticleAdvection_h
