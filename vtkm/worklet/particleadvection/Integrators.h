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

#include <vtkm/TypeTraits.h>
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
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  Integrator()
    : StepLength(0)
    , MinimizeError(false)
  {
  }

  VTKM_EXEC_CONT
  Integrator(const FieldEvaluateType evaluator, const FieldType stepLength)
    : Evaluator(evaluator)
    , StepLength(stepLength)
    , MinimizeError(false)
  {
  }

public:
  VTKM_EXEC
  ParticleStatus Step(const vtkm::Vec<FieldType, 3>& inpos,
                      FieldType& time,
                      vtkm::Vec<FieldType, 3>& outpos) const
  {
    // If without taking the step the particle is out of either spatial
    // or temporal boundary, then return the corresponding status.
    if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    if (!this->Evaluator.IsWithinTemporalBoundary(time))
      return ParticleStatus::EXITED_TEMPORAL_BOUNDARY;

    vtkm::Vec<FieldType, 3> velocity;
    ParticleStatus status = CheckStep(inpos, this->StepLength, time, velocity);
    if (status == ParticleStatus::STATUS_OK)
    {
      outpos = inpos + StepLength * velocity;
      time += StepLength;
    }
    else
    {
      outpos = inpos;
    }
    return status;
  }


  VTKM_EXEC
  ParticleStatus PushOutOfBoundary(vtkm::Vec<FieldType, 3>& inpos,
                                   vtkm::Id numSteps,
                                   FieldType& time,
                                   ParticleStatus status,
                                   vtkm::Vec<FieldType, 3>& outpos) const
  {
    FieldType stepLength = StepLength;
    vtkm::Vec<FieldType, 3> velocity, currentVelocity;
    CheckStep(inpos, 0.0f, time, currentVelocity);
    numSteps = numSteps == 0 ? 1 : numSteps;
    if (MinimizeError)
    {
      //Take short steps and minimize error
      FieldType threshold = StepLength / static_cast<FieldType>(numSteps);
      do
      {
        stepLength /= static_cast<FieldType>(2.0);
        status = CheckStep(inpos, stepLength, time, velocity);
        if (status == ParticleStatus::STATUS_OK)
        {
          outpos = inpos + stepLength * velocity;
          inpos = outpos;
          currentVelocity[0] = velocity[0];
          currentVelocity[1] = velocity[1];
          currentVelocity[2] = velocity[2];
          time += stepLength;
        }
      } while (stepLength > threshold);
    }
    // At this point we have push the point close enough to the boundary
    // so as to minimize the domain switching error.
    // Here we need to analyze if the particle is going out of tempotal bounds
    // or spatial boundary to take the proper step to put the particle out
    // of boundary and stop advecting.
    if (status == AT_SPATIAL_BOUNDARY)
    {
      // Get the spatial boundary w.r.t the current
      vtkm::Vec<FieldType, 3> spatialBoundary;
      Evaluator.GetSpatialBoundary(currentVelocity, spatialBoundary);
      FieldType hx = (vtkm::Abs(spatialBoundary[0] - inpos[0])) / vtkm::Abs(currentVelocity[0]);
      FieldType hy = (vtkm::Abs(spatialBoundary[1] - inpos[1])) / vtkm::Abs(currentVelocity[1]);
      FieldType hz = (vtkm::Abs(spatialBoundary[2] - inpos[2])) / vtkm::Abs(currentVelocity[2]);
      stepLength = vtkm::Min(hx, vtkm::Min(hy, hz)) + Tolerance * stepLength;
      // Calculate the final position of the particle which is supposed to be
      // out of spatial boundary.
      outpos = inpos + stepLength * currentVelocity;
      time += stepLength;
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    }
    else if (status == AT_TEMPORAL_BOUNDARY)
    {
      FieldType temporalBoundary;
      Evaluator.GetTemporalBoundary(temporalBoundary);
      FieldType diff = temporalBoundary - time;
      stepLength = diff + Tolerance * diff;
      // Calculate the final position of the particle which is supposed to be
      // out of temporal boundary.
      outpos = inpos + stepLength * currentVelocity;
      time += stepLength;
      return ParticleStatus::EXITED_TEMPORAL_BOUNDARY;
    }
    //If the control reaches here, it is an invalid case.
    return ParticleStatus::STATUS_ERROR;
  }

  VTKM_EXEC
  ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                           FieldType stepLength,
                           FieldType time,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    using ConcreteType = IntegratorType<FieldEvaluateType, FieldType>;
    return static_cast<const ConcreteType*>(this)->CheckStep(inpos, stepLength, time, velocity);
  }

  VTKM_EXEC_CONT
  FieldType SetMinimizeError(bool minimizeError) const { this->MinimizeError = minimizeError; }

protected:
  FieldEvaluateType Evaluator;
  FieldType StepLength;
  FieldType Tolerance = static_cast<FieldType>(1 / 100.0f);
  bool MinimizeError;
};

template <typename FieldEvaluateType, typename FieldType>
class RK4Integrator : public Integrator<FieldEvaluateType, FieldType, RK4Integrator>
{
public:
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  RK4Integrator()
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::RK4Integrator>()
  {
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, const FieldType stepLength)
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::RK4Integrator>(
        evaluator,
        stepLength)
  {
  }

  VTKM_EXEC
  ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                           FieldType stepLength,
                           FieldType time,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    FieldType var1 = (stepLength / static_cast<FieldType>(2));
    FieldType var2 = time + var1;
    FieldType var3 = time + stepLength;

    vtkm::Vec<FieldType, 3> k1 = vtkm::TypeTraits<vtkm::Vec<FieldType, 3>>::ZeroInitialization();
    vtkm::Vec<FieldType, 3> k2 = k1, k3 = k1, k4 = k1;

    bool status1 = this->Evaluator.Evaluate(inpos, time, k1);
    bool status2 = this->Evaluator.Evaluate(inpos + var1 * k1, var2, k2);
    bool status3 = this->Evaluator.Evaluate(inpos + var1 * k2, var2, k3);
    bool status4 = this->Evaluator.Evaluate(inpos + stepLength * k3, var3, k4);

    if ((status1 & status2 & status3 & status4) == ParticleStatus::STATUS_OK)
    {
      velocity = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
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
  EulerIntegrator()
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::EulerIntegrator>()
  {
  }

  VTKM_EXEC_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, const FieldType stepLength)
    : Integrator<FieldEvaluateType, FieldType, vtkm::worklet::particleadvection::EulerIntegrator>(
        evaluator,
        stepLength)
  {
  }

  VTKM_EXEC
  ParticleStatus CheckStep(const vtkm::Vec<FieldType, 3>& inpos,
                           FieldType vtkmNotUsed(stepLength),
                           FieldType time,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    bool result = this->Evaluator.Evaluate(inpos, time, velocity);
    if (result)
      return ParticleStatus::STATUS_OK;
    else
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
