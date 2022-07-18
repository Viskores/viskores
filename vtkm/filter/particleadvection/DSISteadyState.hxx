//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DSISteadyState_hxx
#define vtk_m_filter_particleadvection_DSISteadyState_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

namespace internal
{
using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using VelocityFieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
using SteadyStateGridEvalType = vtkm::worklet::particleadvection::GridEvaluator<VelocityFieldType>;

template <typename GridEvalType, typename ParticleType>
class AdvectHelper;

template <typename ParticleType>
class AdvectHelper<SteadyStateGridEvalType, ParticleType>
{
public:
  static void Advect(const VelocityFieldType& velField,
                     const vtkm::cont::DataSet& ds,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::ParticleAdvectionResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::RK4Integrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const VelocityFieldType& velField,
                     const vtkm::cont::DataSet& ds,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::StreamlineResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::RK4Integrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const VelocityFieldType& velField,
                       const vtkm::cont::DataSet& ds,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType =
      vtkm::worklet::particleadvection::Stepper<SolverType<SteadyStateGridEvalType>,
                                                SteadyStateGridEvalType>;

    WorkletType worklet;
    SteadyStateGridEvalType eval(ds, velField);
    StepperType stepper(eval, stepSize);
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};
}

VTKM_CONT inline void DSISteadyState::DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                               vtkm::FloatDefault stepSize,
                                               vtkm::Id maxSteps)
{
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
    FieldType velField;
    this->GetVelocityField(velField);

    using AHType = internal::AdvectHelper<internal::SteadyStateGridEvalType, vtkm::Particle>;

    if (this->IsParticleAdvectionResult())
    {
      vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(
        velField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else if (this->IsStreamlineResult())
    {
      vtkm::worklet::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(
        velField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported result type");
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported vector field type");
}

VTKM_CONT inline void DSISteadyState::DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& vtkmNotUsed(b),
                                               vtkm::FloatDefault vtkmNotUsed(stepSize),
                                               vtkm::Id vtkmNotUsed(maxSteps))
{
}

}
}
}

#endif //vtk_m_filter_particleadvection_DSISteadyState_hxx
