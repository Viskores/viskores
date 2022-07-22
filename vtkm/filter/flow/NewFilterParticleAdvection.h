//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_NewFilterParticleAdvection_h
#define vtk_m_filter_flow_NewFilterParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/flow/BoundsMap.h>
#include <vtkm/filter/flow/DataSetIntegrator.h>
#include <vtkm/filter/flow/DataSetIntegratorSteadyState.h>
#include <vtkm/filter/flow/DataSetIntegratorUnsteadyState.h>
#include <vtkm/filter/flow/ParticleAdvectionTypes.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.

class NewFilterParticleAdvection : public vtkm::filter::NewFilterField
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  NewFilterParticleAdvection(vtkm::filter::flow::FlowResultType rType)
    //    : vtkm::filter::NewFilterField()
    : NumberOfSteps(0)
    , ResultType(rType)
    , SolverType(vtkm::filter::flow::IntegrationSolverType::RK4_TYPE)
    , StepSize(0)
    , UseThreadedAlgorithm(false)
    , VecFieldType(vtkm::filter::flow::VectorFieldType::VELOCITY_FIELD_TYPE)
  {
  }

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
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VTKM_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override
  {
    auto out = this->DoExecutePartitions(inData);
    if (out.GetNumberOfPartitions() != 1)
      throw vtkm::cont::ErrorFilterExecution("Wrong number of results");

    return out.GetPartition(0);
  }

  VTKM_CONT virtual void ValidateOptions() const
  {
    if (this->GetUseCoordinateSystemAsField())
      throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");
    if (this->Seeds.GetNumberOfValues() == 0)
      throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
    if (this->NumberOfSteps == 0)
      throw vtkm::cont::ErrorFilterExecution("Number of steps not specified.");
    if (this->StepSize == 0)
      throw vtkm::cont::ErrorFilterExecution("Step size not specified.");
    if (this->NumberOfSteps < 0)
      throw vtkm::cont::ErrorFilterExecution("NumberOfSteps cannot be negative");
    if (this->StepSize < 0)
      throw vtkm::cont::ErrorFilterExecution("StepSize cannot be negative");
  }

  vtkm::Id NumberOfSteps;
  vtkm::filter::flow::FlowResultType ResultType = vtkm::filter::flow::FlowResultType::UNKNOWN_TYPE;
  vtkm::cont::UnknownArrayHandle Seeds;
  vtkm::filter::flow::IntegrationSolverType SolverType;
  vtkm::FloatDefault StepSize;
  bool UseThreadedAlgorithm;
  vtkm::filter::flow::VectorFieldType VecFieldType;

private:
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_NewFilterParticleAdvection_h
