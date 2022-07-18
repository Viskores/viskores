//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_NewFilterParticleAdvection_h
#define vtk_m_filter_NewFilterParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DSI.h>
#include <vtkm/filter/particleadvection/DSISteadyState.h>
#include <vtkm/filter/particleadvection/DSIUnsteadyState.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.

class NewFilterParticleAdvection : public vtkm::filter::NewFilterField
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  NewFilterParticleAdvection(vtkm::filter::particleadvection::ParticleAdvectionResultType rType);

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
  void SetSolverRK4()
  {
    this->SolverType = vtkm::filter::particleadvection::IntegrationSolverType::RK4_TYPE;
  }
  VTKM_CONT
  void SetSolverEuler()
  {
    this->SolverType = vtkm::filter::particleadvection::IntegrationSolverType::EULER_TYPE;
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

  VTKM_CONT inline void ValidateOptions() const;

  vtkm::Id NumberOfSteps;
  vtkm::filter::particleadvection::ParticleAdvectionResultType ResultType =
    vtkm::filter::particleadvection::ParticleAdvectionResultType::UNKNOWN_TYPE;
  vtkm::cont::UnknownArrayHandle Seeds;
  vtkm::filter::particleadvection::IntegrationSolverType SolverType;
  vtkm::FloatDefault StepSize;
  bool UseThreadedAlgorithm;
  vtkm::filter::particleadvection::VectorFieldType VecFieldType;

private:
};


class NewFilterSteadyStateParticleAdvection : public NewFilterParticleAdvection
{
public:
  VTKM_CONT
  NewFilterSteadyStateParticleAdvection(
    vtkm::filter::particleadvection::ParticleAdvectionResultType rType)
    : NewFilterParticleAdvection(rType)
  {
  }

protected:
  VTKM_CONT inline std::vector<vtkm::filter::particleadvection::DSISteadyState*>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::particleadvection::BoundsMap& boundsMap) const;
};

class NewFilterUnsteadyStateParticleAdvection : public NewFilterParticleAdvection
{
public:
  VTKM_CONT
  NewFilterUnsteadyStateParticleAdvection(
    vtkm::filter::particleadvection::ParticleAdvectionResultType rType)
    : NewFilterParticleAdvection(rType)
  {
  }

  void SetPreviousTime(vtkm::FloatDefault t1) { this->Time1 = t1; }
  void SetNextTime(vtkm::FloatDefault t2) { this->Time2 = t2; }
  void SetNextDataSet(const vtkm::cont::DataSet& ds) { this->Input2 = { ds }; }
  void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->Input2 = pds; }

protected:
  VTKM_CONT inline std::vector<vtkm::filter::particleadvection::DSIUnsteadyState*>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::particleadvection::BoundsMap& boundsMap) const;

  vtkm::cont::PartitionedDataSet Input2;
  vtkm::FloatDefault Time1;
  vtkm::FloatDefault Time2;
};

}
} // namespace vtkm::filter

#ifndef vtk_m_filter_NewFilterParticleAdvection_hxx
#include <vtkm/filter/NewFilterParticleAdvection.hxx>
#endif

#endif // vtk_m_filter_NewFilterParticleAdvection_h
