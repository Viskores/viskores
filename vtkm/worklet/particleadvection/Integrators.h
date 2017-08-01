//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

template <typename FieldEvaluateType, typename FieldType, typename PortalType>
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
  ParticleStatus Step(const vtkm::Vec<FieldType, 3>& inpos,
                      const PortalType& field,
                      vtkm::Vec<FieldType, 3>& outpos) const
  {
    vtkm::Vec<FieldType, 3> velocity;
    ParticleStatus status = this->CheckStep(inpos, field, this->StepLength, velocity);
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
  virtual ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                                   const PortalType& field,
                                   FieldType stepLength,
                                   vtkm::Vec<FieldType, 3>& velocity) const = 0;

  VTKM_EXEC
  virtual FieldType GetEscapeStepLength(const vtkm::Vec<FieldType, 3>& inpos,
                                        const PortalType& field,
                                        vtkm::Vec<FieldType, 3>& velocity,
                                        FieldType stepLength) const
  {
    this->CheckStep(inpos, field, stepLength, velocity);
    FieldType magnitude = vtkm::Magnitude(velocity);
    vtkm::Vec<FieldType, 3> dir = velocity / magnitude;
    FieldType xbound = static_cast<FieldType>(dir[0] > 0 ? Bounds.X.Max : Bounds.X.Min);
    FieldType ybound = static_cast<FieldType>(dir[1] > 0 ? Bounds.Y.Max : Bounds.Y.Min);
    FieldType zbound = static_cast<FieldType>(dir[2] > 0 ? Bounds.Z.Max : Bounds.Z.Min);
    /*Add a fraction just push the particle beyond the bounds*/
    FieldType hx = (std::abs(xbound - inpos[0]) + this->Tolerance) / std::abs(velocity[0]);
    FieldType hy = (std::abs(ybound - inpos[1]) + this->Tolerance) / std::abs(velocity[1]);
    FieldType hz = (std::abs(zbound - inpos[2]) + this->Tolerance) / std::abs(velocity[2]);
    stepLength = std::min(hx, std::min(hy, hz));
    return stepLength;
  }

  VTKM_EXEC
  ParticleStatus PushOutOfDomain(vtkm::Vec<FieldType, 3> inpos,
                                 const PortalType& field,
                                 vtkm::Id numSteps,
                                 vtkm::Vec<FieldType, 3>& outpos) const
  {
    numSteps = (numSteps == 0) ? 1 : numSteps;
    FieldType totalTime = numSteps * this->StepLength;
    FieldType timeFraction = totalTime * this->Tolerance;
    FieldType stepLength = this->StepLength / 2;
    vtkm::Vec<FieldType, 3> velocity;
    if (this->ShortStepsSupported)
    {
      do
      {
        ParticleStatus status = this->CheckStep(inpos, field, stepLength, velocity);
        if (status == ParticleStatus::STATUS_OK)
        {
          inpos = inpos + stepLength * velocity;
        }
        stepLength = stepLength / 2;
      } while (stepLength > timeFraction);
    }
    stepLength = GetEscapeStepLength(inpos, field, velocity, stepLength);
    outpos = inpos + stepLength * velocity;
    return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

protected:
  FieldEvaluateType Evaluator;
  FieldType StepLength;
  FieldType Tolerance = static_cast<FieldType>(1e-6);
  bool ShortStepsSupported = false;
  vtkm::Bounds Bounds;
};

template <typename FieldEvaluateType, typename FieldType, typename PortalType>
class RK4Integrator : public Integrator<FieldEvaluateType, FieldType, PortalType>
{
public:
  VTKM_EXEC_CONT
  RK4Integrator()
    : Integrator<FieldEvaluateType, FieldType, PortalType>()
  {
    this->ShortStepsSupported = true;
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& field, FieldType stepLength, vtkm::cont::DataSet& dataset)
    : Integrator<FieldEvaluateType, FieldType, PortalType>(field, stepLength)
  {
    this->ShortStepsSupported = true;
    this->Bounds = dataset.GetCoordinateSystem(0).GetBounds();
  }

  VTKM_EXEC
  virtual ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                                   const PortalType& field,
                                   FieldType stepLength,
                                   vtkm::Vec<FieldType, 3>& velocity) const VTKM_OVERRIDE
  {
    if (!this->Bounds.Contains(inpos))
    {
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    }
    vtkm::Vec<FieldType, 3> k1, k2, k3, k4;
    bool firstOrderValid = this->Evaluator.Evaluate(inpos, field, k1);
    bool secondOrderValid = this->Evaluator.Evaluate(inpos + (stepLength / 2) * k1, field, k2);
    bool thirdOrderValid = this->Evaluator.Evaluate(inpos + (stepLength / 2) * k2, field, k3);
    bool fourthOrderValid = this->Evaluator.Evaluate(inpos + stepLength * k3, field, k4);
    velocity = stepLength / 6.0f * (k1 + 2 * k2 + 2 * k3 + k4);
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

template <typename FieldEvaluateType, typename FieldType, typename PortalType>
class EulerIntegrator : public Integrator<FieldEvaluateType, FieldType, PortalType>
{
public:
  VTKM_EXEC_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, FieldType field, vtkm::cont::DataSet& dataset)
    : Integrator<FieldEvaluateType, FieldType, PortalType>(evaluator, field)
  {
    this->ShortStepsSupported = false;
    this->Bounds = dataset.GetCoordinateSystem(0).GetBounds();
  }

  VTKM_EXEC
  virtual ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                                   const PortalType& field,
                                   vtkm::Vec<FieldType, 3>& velocity) const
  {
    bool validPos = this->Evaluator.Evaluate(inpos, field, velocity);
    if (validPos)
      return ParticleStatus::STATUS_OK;
    else
      return ParticleStatus::AT_SPATIAL_BOUNDARY;
  }

}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
