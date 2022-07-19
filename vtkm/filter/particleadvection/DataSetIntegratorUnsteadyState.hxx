//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DataSetIntegratorUnsteadyState_hxx
#define vtk_m_filter_particleadvection_DataSetIntegratorUnsteadyState_hxx

#include <vtkm/worklet/particleadvection/TemporalGridEvaluators.h>

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
using UnsteadyStateGridEvalType =
  vtkm::worklet::particleadvection::TemporalGridEvaluator<VelocityFieldType>;

template <typename GridEvalType, typename ParticleType>
class AdvectHelper;

template <typename ParticleType>
class AdvectHelper<UnsteadyStateGridEvalType, ParticleType>
{
public:
  static void Advect(const VelocityFieldType& velField1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const VelocityFieldType& velField2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
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
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const VelocityFieldType& velField1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const VelocityFieldType& velField2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
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
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const VelocityFieldType& velField1,
                       const vtkm::cont::DataSet& ds1,
                       vtkm::FloatDefault t1,
                       const VelocityFieldType& velField2,
                       const vtkm::cont::DataSet& ds2,
                       vtkm::FloatDefault t2,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType =
      vtkm::worklet::particleadvection::Stepper<SolverType<UnsteadyStateGridEvalType>,
                                                UnsteadyStateGridEvalType>;

    WorkletType worklet;
    UnsteadyStateGridEvalType eval(ds1, t1, velField1, ds2, t2, velField2);
    StepperType stepper(eval, stepSize);
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};

}

VTKM_CONT inline void DataSetIntegratorUnsteadyState::DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                                               vtkm::FloatDefault stepSize,
                                                               vtkm::Id maxSteps)
{
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

  using AHType = internal::AdvectHelper<internal::UnsteadyStateGridEvalType, vtkm::Particle>;

  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
    FieldType velField1, velField2;
    this->GetVelocityFields(velField1, velField2);

    if (this->IsParticleAdvectionResult())
    {
      vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(velField1,
                     this->DataSet1,
                     this->Time1,
                     velField2,
                     this->DataSet2,
                     this->Time2,
                     seedArray,
                     stepSize,
                     maxSteps,
                     this->SolverType,
                     result);
      this->UpdateResult(result, b);
    }
    else if (this->IsStreamlineResult())
    {
      vtkm::worklet::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(velField1,
                     this->DataSet1,
                     this->Time1,
                     velField2,
                     this->DataSet2,
                     this->Time2,
                     seedArray,
                     stepSize,
                     maxSteps,
                     this->SolverType,
                     result);
      this->UpdateResult(result, b);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported result type");
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported vector field type");
}

VTKM_CONT inline void DataSetIntegratorUnsteadyState::DoAdvect(
  DSIHelperInfo<vtkm::ChargedParticle>& vtkmNotUsed(b),
  vtkm::FloatDefault vtkmNotUsed(stepSize),
  vtkm::Id vtkmNotUsed(maxSteps))
{
}

}
}
}

#endif //vtk_m_filter_particleadvection_DataSetIntegratorUnsteadyState_hxx
