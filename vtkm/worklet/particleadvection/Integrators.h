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

template <typename FieldEvaluateType, typename FieldType>
class RK4Integrator
{
public:
  VTKM_EXEC_CONT
  RK4Integrator()
    : StepLength(0)
  {
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, FieldType stepLength)
    : Evaluator(evaluator)
    , StepLength(stepLength)
  {
  }

  VTKM_EXEC
  ParticleStatus Step(const vtkm::Vec<FieldType, 3>& inpos,
                      FieldType time,
                      vtkm::Vec<FieldType, 3>& outpos) const
  {
    vtkm::Vec<FieldType, 3> velocity;
    ParticleStatus status = this->CheckStep(inpos, this->StepLength, time, velocity);
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
                           FieldType time,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    vtkm::Vec<FieldType, 3> k1, k2, k3, k4;

    bool status1 = Evaluator.Evaluate(inpos, time, k1);
    bool status2 = Evaluator.Evaluate(inpos + (stepLength / 2) * k1, time, k2);
    bool status3 = Evaluator.Evaluate(inpos + (stepLength / 2) * k2, time, k3);
    bool status4 = Evaluator.Evaluate(inpos + stepLength * k3, time, k4);

    if (status1 & status2 & status3 & status4 == ParticleStatus::STATUS_OK)
    {
      velocity = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
      return ParticleStatus::STATUS_OK;
    }
    else
    {
      return ParticleStatus::AT_SPATIAL_BOUNDARY;
    }
  }

private:
  FieldEvaluateType Evaluator;
  FieldType StepLength;
};

template <typename FieldEvaluateType, typename FieldType>
class EulerIntegrator
{
public:
  VTKM_EXEC_CONT
  EulerIntegrator()
    : StepLength(0)
  {
  }

  VTKM_EXEC_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, FieldType stepLength)
    : Evaluator(evaluator)
    , StepLength(stepLength)
  {
  }

  VTKM_EXEC
  ParticleStatus Step(const vtkm::Vec<FieldType, 3>& inpos,
                      FieldType time,
                      vtkm::Vec<FieldType, 3>& outpos) const
  {
    vtkm::Vec<FieldType, 3> velocity;
    ParticleStatus status = CheckStep(inpos, StepLength, time, velocity);
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
                           FieldType time,
                           vtkm::Vec<FieldType, 3>& velocity) const
  {
    if (!Evaluator.IsWithinSpatialBoundary(inpos))
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    if (!Evaluator.IsWithinTemporalBoundary(time))
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;

    bool result = this->Evaluator.Evaluate(inpos, time, velocity);
    if (result)
      return ParticleStatus::STATUS_OK;
    else
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

private:
  FieldEvaluateType Evaluator;
  FieldType StepLength;
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
