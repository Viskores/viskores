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
                                        FieldType stepLength) const = 0;

  VTKM_EXEC
  ParticleStatus PushOutOfDomain(vtkm::Vec<FieldType, 3> inpos,
                                 const PortalType& field,
                                 vtkm::Vec<FieldType, 3>& outpos) const
  {
    FieldType stepLength = this->StepLength / 2;
    vtkm::Vec<FieldType, 3> velocity;
    vtkm::Id numSteps = 0;
    do
    {
      ParticleStatus status = this->CheckStep(inpos, field, stepLength, velocity);
      if (status == ParticleStatus::STATUS_OK)
      {
        inpos = inpos + stepLength * velocity;
        ++numSteps;
      }
      stepLength = stepLength / 2;
    } while (numSteps < 10);
    stepLength = GetEscapeStepLength(inpos, field, stepLength);
    outpos = inpos + stepLength * velocity;
    return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

protected:
  FieldEvaluateType Evaluator;
  FieldType StepLength;
};

template <typename FieldEvaluateType, typename FieldType, typename PortalType>
class RK4Integrator : public Integrator<FieldEvaluateType, FieldType, PortalType>
{
public:
  VTKM_EXEC_CONT
  RK4Integrator()
    : Integrator<FieldEvaluateType, FieldType, PortalType>()
  {
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& field, FieldType stepLength, vtkm::cont::DataSet& dataset)
    : Integrator<FieldEvaluateType, FieldType, PortalType>(field, stepLength)
  {
    bounds = dataset.GetCoordinateSystem(0).GetBounds();
  }

  VTKM_EXEC
  virtual ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                                   const PortalType& field,
                                   FieldType stepLength,
                                   vtkm::Vec<FieldType, 3>& velocity) const VTKM_OVERRIDE
  {
    if (!bounds.Contains(inpos))
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

  VTKM_EXEC
  virtual FieldType GetEscapeStepLength(const vtkm::Vec<FieldType, 3>& inpos,
                                        const PortalType& field,
                                        FieldType stepLength) const VTKM_OVERRIDE
  {
    vtkm::Vec<FieldType, 3> velocity;
    this->CheckStep(inpos, field, stepLength, velocity);
    FieldType magnitude = vtkm::Magnitude(velocity);
    vtkm::Vec<FieldType, 3> dir = velocity / magnitude;
    FieldType xbound = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
    FieldType ybound = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
    FieldType zbound = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
    FieldType hx = std::abs(xbound - inpos[0]) / std::abs(velocity[0]);
    FieldType hy = std::abs(ybound - inpos[1]) / std::abs(velocity[1]);
    FieldType hz = std::abs(zbound - inpos[2]) / std::abs(velocity[2]);
    stepLength = std::min(hx, std::min(hy, hz));
    stepLength += stepLength / 100.0f;
    return stepLength;
  }


private:
  vtkm::Bounds bounds;
};

/*template<typename FieldEvaluateType, typename FieldType>
class RK4Integrator
{
public:
  VTKM_EXEC_CONT
  RK4Integrator()
    : h(0)
  {
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& field, FieldType _h, vtkm::cont::DataSet& dataset)
    : f(field)
    , h(_h)
  {
    bounds = dataset.GetCoordinateSystem(0).GetBounds();
  }

  template<typename PortalType>
  VTKM_EXEC ParticleStatus Step(const vtkm::Vec<FieldType, 3>& pos,
                                const PortalType& field,
                                vtkm::Vec<FieldType, 3>& out) const
  {
    if(!bounds.Contains(pos))
    {
      out[0] = pos[0]; out[1] = pos[1]; out[2] = pos[2];
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    }
    vtkm::Vec<FieldType, 3> k1, k2, k3, k4;
    vtkm::Id numShortSteps = 0;
    bool shortSteps = false;
    FieldType step_h = h;
    FieldType step_h_2 = step_h / 2;
    if (f.Evaluate(pos, field, k1) && f.Evaluate(pos + step_h_2 * k1, field, k2) &&
        f.Evaluate(pos + step_h_2 * k2, field, k3) && f.Evaluate(pos + step_h * k3, field, k4))
    {
      *
       * If the particle is inside bounds after taking the steps
       * return that the step was successful.
       *
      out = pos + step_h / 6.0f * (k1 + 2 * k2 + 2 * k3 + k4);
      return ParticleStatus::STATUS_OK;
    }
    else
    {
      shortSteps = true;
    }
    *Take short steps to minimize advection error*
    while (shortSteps)
    {
      *reduce the step size to half*
      step_h /= 2;
      step_h_2 = step_h / 2;
      if (f.Evaluate(pos, field, k1) && f.Evaluate(pos + step_h_2 * k1, field, k2) &&
          f.Evaluate(pos + step_h_2 * k2, field, k3) && f.Evaluate(pos + step_h * k3, field, k4))
      {
        out = pos + step_h / 6.0f * (k1 + 2 * k2 + 2 * k3 + k4);
        numShortSteps++;
      }
      *
        * At this time the particle is still inside the bounds
        * To Push it at/out of the boundary take an Euler step
        *
      *Check for the function like VisIt*
      if (numShortSteps == 2)
      {
        step_h /= 2;
        step_h_2 = step_h / 2;
        *Calculate the velocity of the particle at current position*
        f.Evaluate(out, field, k1);
        f.Evaluate(out + step_h_2 * k1, field, k2);
        f.Evaluate(out + step_h_2 * k2, field, k3);
        f.Evaluate(out + step_h * k3, field, k4);
        vtkm::Vec<FieldType, 3> vel = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
        *Get the direction of the particle*
        FieldType magnitude = vtkm::Magnitude(vel);
        vtkm::Vec<FieldType, 3> dir = vel / magnitude;
        *Determine the bounds for the particle*
        FieldType xbound = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
        FieldType ybound = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
        FieldType zbound = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
        *
         * Determine the minimum travel time for the
         * particle to reach the boundary
         *
        FieldType hx = std::abs(xbound - out[0]) / std::abs(vel[0]);
        FieldType hy = std::abs(ybound - out[1]) / std::abs(vel[1]);
        FieldType hz = std::abs(zbound - out[2]) / std::abs(vel[2]);
        FieldType hesc = std::min(hx, std::min(hy, hz));
        hesc += hesc / 100.0f;
        out += hesc * vel;
        shortSteps = false;
        break;
      }
    }
    return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

  FieldEvaluateType f;
  FieldType h;
  vtkm::Bounds bounds;
};*/ //RK4Integrator

/*template<typename FieldEvaluateType, typename FieldType>
class EulerIntegrator
{
public:
  VTKM_EXEC_CONT
  EulerIntegrator(const FieldEvaluateType& field, FieldType _h)
    : f(field)
    , h(_h)
  {
  }

  template<typename PortalType>
  VTKM_EXEC ParticleStatus Step(const vtkm::Vec<FieldType, 3>& pos,
                      const PortalType& field,
                      vtkm::Vec<FieldType, 3>& out) const
  {
    vtkm::Vec<FieldType, 3> vCur;
    if (f.Evaluate(pos, field, vCur))
    {
      out = pos + h * vCur;
      return ParticleStatus::STATUS::OK;
    }
    return ParticleStatus::STATUS::EXITED_SPATIAL_BOUNDARY;
  }

  FieldEvaluateType f;
  FieldType h;
};*/ //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
