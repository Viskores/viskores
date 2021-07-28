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

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename ExecEvaluatorType>
class ExecRK4Integrator
{
public:
  VTKM_EXEC_CONT
  ExecRK4Integrator(const ExecEvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  template <typename Particle>
  VTKM_EXEC IntegratorStatus CheckStep(Particle& particle,
                                       vtkm::FloatDefault stepLength,
                                       vtkm::Vec3f& velocity) const
  {
    auto time = particle.Time;
    auto inpos = particle.Pos;
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
    v1 = particle.Velocity(k1, stepLength);

    evalStatus = this->Evaluator.Evaluate(inpos + var1 * v1, var2, k2);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus);
    v2 = particle.Velocity(k2, stepLength);

    evalStatus = this->Evaluator.Evaluate(inpos + var1 * v2, var2, k3);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus);
    v3 = particle.Velocity(k3, stepLength);

    evalStatus = this->Evaluator.Evaluate(inpos + stepLength * v3, var3, k4);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus);
    v4 = particle.Velocity(k4, stepLength);

    velocity = (v1 + 2 * v2 + 2 * v3 + v4) / static_cast<vtkm::FloatDefault>(6);

    return IntegratorStatus(evalStatus);
  }

private:
  ExecEvaluatorType Evaluator;
};

template <typename EvaluatorType>
class RK4Integrator
{
private:
  EvaluatorType Evaluator;

public:
  VTKM_CONT
  RK4Integrator() = default;

  VTKM_CONT
  RK4Integrator(const EvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  VTKM_CONT auto PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
    -> ExecRK4Integrator<decltype(this->Evaluator.PrepareForExecution(device, token))>
  {
    auto evaluator = this->Evaluator.PrepareForExecution(device, token);
    using ExecEvaluatorType = decltype(evaluator);
    return ExecRK4Integrator<ExecEvaluatorType>(evaluator);
  }
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_RK4Integrator_h
