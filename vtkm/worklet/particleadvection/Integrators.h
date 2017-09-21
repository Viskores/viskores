//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_Integrators_h
#define vtk_m_worklet_particleadvection_Integrators_h

#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename FieldEvaluateType,
          typename FieldType,
          template <typename, typename> class IntegratorType>
class Integrator
{
protected:
  VTKM_EXEC_CONT
  Integrator()
    : StepLength(0)
  {
  }

  VTKM_EXEC_CONT
  Integrator(const FieldEvaluateType& evaluator, FieldType stepLength)
    : Evaluator(evaluator)
    , StepLength(stepLength)
  {
  }

public:
  VTKM_EXEC
  ParticleStatus Step(const vtkm::Vec<FieldType, 3>& inpos, vtkm::Vec<FieldType, 3>& outpos) const
  {
    vtkm::Vec<FieldType, 3> velocity;
    ParticleStatus status = this->CheckStep(inpos, this->StepLength, velocity);
    if (status == ParticleStatus::STATUS_OK)
    {
      outpos = inpos + this->StepLength * velocity;
    }
    else
    {
      outpos = inpos;
    }
    return status;
  }

  VTKM_EXEC
  ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                           FieldType stepLength,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    using ConcreteType = IntegratorType<FieldEvaluateType, FieldType>;
    return static_cast<const ConcreteType*>(this)->CheckStep(inpos, stepLength, velocity);
  }

  VTKM_EXEC
  ParticleStatus GetEscapeStepLength(const vtkm::Vec<FieldType, 3>& inpos,
                                     FieldType& stepLength,
                                     vtkm::Vec<FieldType, 3>& velocity) const
  {
    ParticleStatus status = this->CheckStep(inpos, stepLength, velocity);
    if (status != ParticleStatus::STATUS_OK)
    {
      stepLength += this->Tolerance;
      return status;
    }
    FieldType magnitude = vtkm::Magnitude(velocity);
    vtkm::Vec<FieldType, 3> dir = velocity / magnitude;
    vtkm::Vec<FieldType, 3> dirBounds;
    this->Evaluator.GetBoundary(dir, dirBounds);
    /*Add a fraction just push the particle beyond the bounds*/
    FieldType hx = (vtkm::Abs(dirBounds[0] - inpos[0]) + this->Tolerance) / vtkm::Abs(velocity[0]);
    FieldType hy = (vtkm::Abs(dirBounds[1] - inpos[1]) + this->Tolerance) / vtkm::Abs(velocity[1]);
    FieldType hz = (vtkm::Abs(dirBounds[2] - inpos[2]) + this->Tolerance) / vtkm::Abs(velocity[2]);
    stepLength = vtkm::Min(hx, vtkm::Min(hy, hz));
    return status;
  }

  VTKM_EXEC
  ParticleStatus PushOutOfDomain(const vtkm::Vec<FieldType, 3>& inpos,
                                 vtkm::Id numSteps,
                                 vtkm::Vec<FieldType, 3>& outpos) const
  {
    ParticleStatus status;
    outpos = inpos;
    numSteps = (numSteps == 0) ? 1 : numSteps;
    FieldType totalTime = static_cast<FieldType>(numSteps) * this->StepLength;
    FieldType timeFraction = totalTime * this->Tolerance;
    FieldType stepLength = this->StepLength / 2;
    vtkm::Vec<FieldType, 3> velocity, currentVelocity;
    this->CheckStep(inpos, 0.0f, currentVelocity);
    if (this->ShortStepsSupported)
    {
      do
      {
        status = this->CheckStep(inpos, stepLength, velocity);
        if (status == ParticleStatus::STATUS_OK)
        {
          outpos = outpos + stepLength * velocity;
          currentVelocity = velocity;
        }
        stepLength = stepLength / 2;
      } while (stepLength > timeFraction);
    }
    status = GetEscapeStepLength(inpos, stepLength, velocity);
    if (status != ParticleStatus::STATUS_OK)
      outpos = outpos + stepLength * currentVelocity;
    else
      outpos = outpos + stepLength * velocity;
    return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

protected:
  FieldEvaluateType Evaluator;
  FieldType StepLength;
  FieldType Tolerance = static_cast<FieldType>(1e-6);
  bool ShortStepsSupported = false;
};

template <typename FieldEvaluateType, typename FieldType>
class RK4Integrator : public Integrator<FieldEvaluateType, FieldType, RK4Integrator>
{
public:
  VTKM_EXEC_CONT
  RK4Integrator()
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::RK4Integrator>()
  {
    this->ShortStepsSupported = true;
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, FieldType stepLength)
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::RK4Integrator>(
        evaluator,
        stepLength)
  {
    this->ShortStepsSupported = true;
  }

  VTKM_EXEC
  ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                           FieldType stepLength,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    if (!this->Evaluator.IsWithinBoundary(inpos))
    {
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    }
    vtkm::Vec<FieldType, 3> k1, k2, k3, k4;
    bool firstOrderValid = this->Evaluator.Evaluate(inpos, k1);
    bool secondOrderValid = this->Evaluator.Evaluate(inpos + (stepLength / 2) * k1, k2);
    bool thirdOrderValid = this->Evaluator.Evaluate(inpos + (stepLength / 2) * k2, k3);
    bool fourthOrderValid = this->Evaluator.Evaluate(inpos + stepLength * k3, k4);
    velocity = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
    if (firstOrderValid && secondOrderValid && thirdOrderValid && fourthOrderValid)
    {
      return ParticleStatus::STATUS_OK;
    }
    else
    {
      return ParticleStatus::AT_SPATIAL_BOUNDARY;
    }
  }
};

template <typename FieldEvaluateType, typename FieldType>
class EulerIntegrator : public Integrator<FieldEvaluateType, FieldType, EulerIntegrator>
{
public:
  VTKM_EXEC_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, FieldType field)
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::EulerIntegrator>(
        evaluator,
        field)
  {
    this->ShortStepsSupported = false;
  }

  VTKM_EXEC
  ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                           FieldType stepLength,
                           vtkm::Vec<FieldType, 3>& velocity)
  {
    stepLength = 0;
    bool isValidPos = this->Evaluator.Evaluate(inpos, velocity);
    if (isValidPos)
      return ParticleStatus::STATUS_OK;
    else
      return ParticleStatus::AT_SPATIAL_BOUNDARY;
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
