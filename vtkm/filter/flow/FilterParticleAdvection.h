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
#include <vtkm/cont/ErrorFilterExecution.h>
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

  VTKM_CONT void SetBlockIDs(const std::vector<vtkm::Id>& blockIds)
  {
    this->BlockIds = blockIds;
    this->BlockIdsSet = true;
  }

  VTKM_CONT
  void SetSolverRK4() { this->SolverType = vtkm::filter::flow::IntegrationSolverType::RK4_TYPE; }

  VTKM_CONT
  void SetSolverEuler()
  {
    this->SolverType = vtkm::filter::flow::IntegrationSolverType::EULER_TYPE;
  }

  VTKM_CONT
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VTKM_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

  VTKM_CONT
  void SetUseAsynchronousCommunication() { this->UseAsynchronousCommunication = true; }
  VTKM_CONT
  bool GetUseAsynchronousCommunication() { return this->UseAsynchronousCommunication; }

  VTKM_CONT
  void SetUseSynchronousCommunication() { this->UseAsynchronousCommunication = false; }
  VTKM_CONT
  bool GetUseSynchronousCommunication() { return !this->GetUseAsynchronousCommunication(); }

protected:
  VTKM_CONT virtual void ValidateOptions() const;

  bool BlockIdsSet = false;
  std::vector<vtkm::Id> BlockIds;

  vtkm::Id NumberOfSteps = 0;
  vtkm::cont::UnknownArrayHandle Seeds;
  vtkm::filter::flow::IntegrationSolverType SolverType =
    vtkm::filter::flow::IntegrationSolverType::RK4_TYPE;
  vtkm::FloatDefault StepSize = 0;
  bool UseAsynchronousCommunication = true;
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
