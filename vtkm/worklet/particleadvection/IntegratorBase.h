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

#ifndef vtk_m_worklet_particleadvection_IntegratorBase_h
#define vtk_m_worklet_particleadvection_IntegratorBase_h

#include <limits>

#include <vtkm/Bitset.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/IntegratorStatus.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

class IntegratorBase : public vtkm::cont::ExecutionObjectBase
{
protected:
  VTKM_CONT
  IntegratorBase() = default;

  VTKM_CONT
  IntegratorBase(vtkm::FloatDefault stepLength)
    : StepLength(stepLength)
  {
  }

public:
  class ExecObject : public vtkm::VirtualObjectBase
  {
  protected:
    VTKM_EXEC_CONT
    ExecObject(const vtkm::FloatDefault stepLength, vtkm::FloatDefault tolerance)
      : StepLength(stepLength)
      , Tolerance(tolerance)
    {
    }

  public:
    VTKM_EXEC
    virtual IntegratorStatus Step(vtkm::Particle* inpos,
                                  vtkm::FloatDefault& time,
                                  vtkm::Vec3f& outpos) const = 0;

    VTKM_EXEC
    virtual IntegratorStatus SmallStep(vtkm::Particle* inpos,
                                       vtkm::FloatDefault& time,
                                       vtkm::Vec3f& outpos) const = 0;

  protected:
    vtkm::FloatDefault StepLength = 1.0f;
    vtkm::FloatDefault Tolerance = 0.001f;
  };

  template <typename Device>
  VTKM_CONT const ExecObject* PrepareForExecution(Device, vtkm::cont::Token& token) const
  {
    this->PrepareForExecutionImpl(
      Device(),
      const_cast<vtkm::cont::VirtualObjectHandle<ExecObject>&>(this->ExecObjectHandle),
      token);
    return this->ExecObjectHandle.PrepareForExecution(Device(), token);
  }

private:
  vtkm::cont::VirtualObjectHandle<ExecObject> ExecObjectHandle;

protected:
  vtkm::FloatDefault StepLength;
  vtkm::FloatDefault Tolerance =
    std::numeric_limits<vtkm::FloatDefault>::epsilon() * static_cast<vtkm::FloatDefault>(100.0f);

  VTKM_CONT virtual void PrepareForExecutionImpl(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::VirtualObjectHandle<ExecObject>& execObjectHandle,
    vtkm::cont::Token& token) const = 0;

  template <typename FieldEvaluateType, typename DerivedType>
  class ExecObjectBaseImpl : public ExecObject
  {
  protected:
    VTKM_EXEC_CONT
    ExecObjectBaseImpl(const FieldEvaluateType& evaluator,
                       vtkm::FloatDefault stepLength,
                       vtkm::FloatDefault tolerance)
      : ExecObject(stepLength, tolerance)
      , Evaluator(evaluator)
    {
    }

  public:
    VTKM_EXEC
    IntegratorStatus Step(vtkm::Particle* particle,
                          vtkm::FloatDefault& time,
                          vtkm::Vec3f& outpos) const override
    {
      // If particle is out of either spatial or temporal boundary to begin with,
      // then return the corresponding status.
      IntegratorStatus status;
      if (!this->Evaluator.IsWithinSpatialBoundary(particle->Pos))
      {
        status.SetFail();
        status.SetSpatialBounds();
        return status;
      }
      if (!this->Evaluator.IsWithinTemporalBoundary(particle->Time))
      {
        status.SetFail();
        status.SetTemporalBounds();
        return status;
      }

      vtkm::Vec3f velocity;
      status = CheckStep(particle, this->StepLength, velocity);
      if (status.CheckOk())
      {
        outpos = particle->Pos + this->StepLength * velocity;
        time += this->StepLength;
      }
      else
        outpos = particle->Pos;

      return status;
    }

    VTKM_EXEC
    IntegratorStatus SmallStep(vtkm::Particle* particle,
                               vtkm::FloatDefault& time,
                               vtkm::Vec3f& outpos) const override
    {
      if (!this->Evaluator.IsWithinSpatialBoundary(particle->Pos))
      {
        outpos = particle->Pos;
        return IntegratorStatus(false, true, false);
      }
      if (!this->Evaluator.IsWithinTemporalBoundary(particle->Time))
      {
        outpos = particle->Pos;
        return IntegratorStatus(false, false, true);
      }

      //Stepping by this->StepLength goes beyond the bounds of the dataset.
      //We need to take an Euler step that goes outside of the dataset.
      //Use a binary search to find the largest step INSIDE the dataset.
      //Binary search uses a shrinking bracket of inside / outside, so when
      //we terminate, the outside value is the stepsize that will nudge
      //the particle outside the dataset.

      //The binary search will be between {0, this->StepLength}
      vtkm::FloatDefault stepRange[2] = { 0, this->StepLength };

      vtkm::Vec3f currPos(particle->Pos), currVelocity;
      vtkm::VecVariable<vtkm::Vec3f, 2> currValue, tmp;
      auto evalStatus = this->Evaluator.Evaluate(currPos, particle->Time, currValue);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);

      const vtkm::FloatDefault eps = vtkm::Epsilon<vtkm::FloatDefault>() * 10;
      vtkm::FloatDefault div = 1;
      while ((stepRange[1] - stepRange[0]) > eps)
      {
        //Try a step midway between stepRange[0] and stepRange[1]
        div *= 2;
        vtkm::FloatDefault stepCurr = stepRange[0] + (this->StepLength / div);

        //See if we can step by stepCurr.
        IntegratorStatus status = this->CheckStep(particle, stepCurr, currVelocity);

        if (status.CheckOk()) //Integration step succedded.
        {
          //See if this point is in/out.
          auto newPos = particle->Pos + stepCurr * currVelocity;
          evalStatus = this->Evaluator.Evaluate(newPos, particle->Time + stepCurr, tmp);
          if (evalStatus.CheckOk())
          {
            //Point still in. Update currPos and set range to {stepCurr, stepRange[1]}
            currPos = newPos;
            stepRange[0] = stepCurr;
          }
          else
          {
            //The step succedded, but the next point is outside.
            //Step too long. Set range to: {stepRange[0], stepCurr} and continue.
            stepRange[1] = stepCurr;
          }
        }
        else
        {
          //Step too long. Set range to: {stepRange[0], stepCurr} and continue.
          stepRange[1] = stepCurr;
        }
      }
      evalStatus = this->Evaluator.Evaluate(currPos, particle->Time + stepRange[0], currValue);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      //Update the position and time.
      outpos = currPos + stepRange[1] * particle->Velocity(currValue, stepRange[1]);
      time += stepRange[1];

      return IntegratorStatus(
        true, true, !this->Evaluator.IsWithinTemporalBoundary(particle->Time));
    }

    VTKM_EXEC
    IntegratorStatus CheckStep(vtkm::Particle* particle,
                               vtkm::FloatDefault stepLength,
                               vtkm::Vec3f& velocity) const
    {
      return static_cast<const DerivedType*>(this)->CheckStep(particle, stepLength, velocity);
    }

  protected:
    FieldEvaluateType Evaluator;
  };
};

namespace detail
{

template <template <typename> class IntegratorType>
struct IntegratorPrepareForExecutionFunctor
{
  template <typename Device, typename EvaluatorType>
  VTKM_CONT bool operator()(
    Device,
    vtkm::cont::VirtualObjectHandle<IntegratorBase::ExecObject>& execObjectHandle,
    const EvaluatorType& evaluator,
    vtkm::FloatDefault stepLength,
    vtkm::FloatDefault tolerance,
    vtkm::cont::Token& token) const
  {
    IntegratorType<Device>* integrator = new IntegratorType<Device>(
      evaluator.PrepareForExecution(Device(), token), stepLength, tolerance);
    execObjectHandle.Reset(integrator);
    return true;
  }
};

} //namespace detail
} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_IntegratorBase_h
