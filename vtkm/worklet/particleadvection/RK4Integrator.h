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

#ifndef vtk_m_worklet_particleadvection_RK4Integrator_h
#define vtk_m_worklet_particleadvection_RK4Integrator_h

#include <vtkm/worklet/particleadvection/IntegratorBase.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename FieldEvaluateType>
class RK4Integrator : public IntegratorBase
{
public:
  VTKM_CONT
  RK4Integrator() = default;

  VTKM_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, vtkm::FloatDefault stepLength)
    : IntegratorBase(stepLength)
    , Evaluator(evaluator)
  {
  }

  template <typename Device>
  class ExecObject
    : public IntegratorBase::ExecObjectBaseImpl<
        vtkm::cont::internal::ExecutionObjectType<FieldEvaluateType, Device>,
        typename RK4Integrator::template ExecObject<Device>>
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using FieldEvaluateExecType =
      vtkm::cont::internal::ExecutionObjectType<FieldEvaluateType, Device>;
    using Superclass =
      IntegratorBase::ExecObjectBaseImpl<FieldEvaluateExecType,
                                         typename RK4Integrator::template ExecObject<Device>>;

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
      vtkm::FloatDefault boundary = this->Evaluator.GetTemporalBoundary(static_cast<vtkm::Id>(1));
      if ((time + stepLength + vtkm::Epsilon<vtkm::FloatDefault>() - boundary) > 0.0)
        stepLength = boundary - time;

      //k1 = F(p,t)
      //k2 = F(p+hk1/2, t+h/2
      //k3 = F(p+hk2/2, t+h/2
      //k4 = F(p+hk3, t+h)
      //Yn+1 = Yn + 1/6 h (k1+2k2+2k3+k4)

      vtkm::FloatDefault var1 = (stepLength / static_cast<vtkm::FloatDefault>(2));
      vtkm::FloatDefault var2 = time + var1;
      vtkm::FloatDefault var3 = time + stepLength;

      vtkm::Vec3f v1 = vtkm::TypeTraits<vtkm::Vec3f>::ZeroInitialization();
      vtkm::Vec3f v2 = v1, v3 = v1, v4 = v1;
      vtkm::VecVariable<vtkm::Vec3f, 2> k1, k2, k3, k4;

      GridEvaluatorStatus evalStatus;

      evalStatus = this->Evaluator.Evaluate(inpos, time, k1);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      v1 = particle->Velocity(k1, stepLength);

      evalStatus = this->Evaluator.Evaluate(inpos + var1 * v1, var2, k2);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      v2 = particle->Velocity(k2, stepLength);

      evalStatus = this->Evaluator.Evaluate(inpos + var1 * v2, var2, k3);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      v3 = particle->Velocity(k3, stepLength);

      evalStatus = this->Evaluator.Evaluate(inpos + stepLength * v3, var3, k4);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      v4 = particle->Velocity(k4, stepLength);

      velocity = (v1 + 2 * v2 + 2 * v3 + v4) / static_cast<vtkm::FloatDefault>(6);
      return IntegratorStatus(true, false, evalStatus.CheckTemporalBounds());
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
};


} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_RK4Integrator_h
