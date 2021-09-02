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

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename EvaluatorType>
class ExecEulerIntegrator
{
public:
  VTKM_EXEC_CONT
  ExecEulerIntegrator(const EvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  template <typename Particle>
  VTKM_EXEC IntegratorStatus CheckStep(Particle& particle,
                                       vtkm::FloatDefault stepLength,
                                       vtkm::Vec3f& velocity) const
  {
    auto time = particle.Time;
    auto inpos = particle.GetEvaluationPosition(stepLength);
    vtkm::VecVariable<vtkm::Vec3f, 2> vectors;
    GridEvaluatorStatus status = this->Evaluator.Evaluate(inpos, time, vectors);
    if (status.CheckOk())
      velocity = particle.Velocity(vectors, stepLength);
    return IntegratorStatus(status);
  }

private:
  EvaluatorType Evaluator;
};

template <typename EvaluatorType>
class EulerIntegrator
{
private:
  EvaluatorType Evaluator;

public:
  EulerIntegrator() = default;

  VTKM_CONT
  EulerIntegrator(const EvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  VTKM_CONT auto PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
    -> ExecEulerIntegrator<decltype(this->Evaluator.PrepareForExecution(device, token))>
  {
    auto evaluator = this->Evaluator.PrepareForExecution(device, token);
    using ExecEvaluatorType = decltype(evaluator);
    return ExecEulerIntegrator<ExecEvaluatorType>(evaluator);
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_EulerIntegrator_h
