//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtk_m_worklet_particleadvection_EulerIntegrator_h
#define vtk_m_worklet_particleadvection_EulerIntegrator_h

#include <vtkm/worklet/particleadvection/IntegratorBase.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename FieldEvaluateType>
class EulerIntegrator : public IntegratorBase
{
public:
  EulerIntegrator() = default;

  VTKM_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, const vtkm::FloatDefault stepLength)
    : IntegratorBase(stepLength)
    , Evaluator(evaluator)
  {
  }

  template <typename Device>
  class ExecObject
    : public IntegratorBase::ExecObjectBaseImpl<
        vtkm::cont::internal::ExecutionObjectType<FieldEvaluateType, Device>,
        typename EulerIntegrator::template ExecObject<Device>>
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using FieldEvaluateExecType =
      vtkm::cont::internal::ExecutionObjectType<FieldEvaluateType, Device>;
    using Superclass =
      IntegratorBase::ExecObjectBaseImpl<FieldEvaluateExecType,
                                         typename EulerIntegrator::template ExecObject<Device>>;

  public:
    VTKM_EXEC_CONT
    ExecObject(const FieldEvaluateExecType& evaluator,
               vtkm::FloatDefault stepLength,
               vtkm::FloatDefault tolerance)
      : Superclass(evaluator, stepLength, tolerance)
    {
    }

    VTKM_EXEC
    IntegratorStatus CheckStep(vtkm::Particle* particle,
                               vtkm::FloatDefault stepLength,
                               vtkm::Vec3f& velocity) const
    {
      auto time = particle->Time;
      auto inpos = particle->Pos;
      vtkm::VecVariable<vtkm::Vec3f, 2> vectors;
      GridEvaluatorStatus status = this->Evaluator.Evaluate(inpos, time, vectors);
      velocity = particle->Velocity(vectors, stepLength);
      return IntegratorStatus(status);
    }
  };

private:
  FieldEvaluateType Evaluator;

protected:
  VTKM_CONT virtual void PrepareForExecutionImpl(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::VirtualObjectHandle<IntegratorBase::ExecObject>& execObjectHandle,
    vtkm::cont::Token& token) const override
  {
    vtkm::cont::TryExecuteOnDevice(device,
                                   detail::IntegratorPrepareForExecutionFunctor<ExecObject>(),
                                   execObjectHandle,
                                   this->Evaluator,
                                   this->StepLength,
                                   this->Tolerance,
                                   token);
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_EulerIntegrator_h
