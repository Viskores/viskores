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

#ifndef vtk_m_worklet_particleadvection_Integrators_h
#define vtk_m_worklet_particleadvection_Integrators_h

#include <limits>

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/worklet/particleadvection/EvaluatorStatus.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{
enum class IntegratorStatus
{
  SUCCESS = 0,
  OUTSIDE_SPATIAL_BOUNDS,
  OUTSIDE_TEMPORAL_BOUNDS,
  FAIL
};

class Integrator : public vtkm::cont::ExecutionObjectBase
{
protected:
  VTKM_CONT
  Integrator() = default;

  VTKM_CONT
  Integrator(vtkm::FloatDefault stepLength)
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
    virtual IntegratorStatus Step(const vtkm::Vec3f& inpos,
                                  vtkm::FloatDefault& time,
                                  vtkm::Vec3f& outpos) const = 0;

    VTKM_EXEC
    virtual IntegratorStatus SmallStep(vtkm::Vec3f& inpos,
                                       vtkm::FloatDefault& time,
                                       vtkm::Vec3f& outpos) const = 0;

    VTKM_EXEC
    IntegratorStatus ConvertToIntegratorStatus(EvaluatorStatus status) const
    {
      switch (status)
      {
        case EvaluatorStatus::SUCCESS:
          return IntegratorStatus::SUCCESS;
        case EvaluatorStatus::OUTSIDE_SPATIAL_BOUNDS:
          return IntegratorStatus::OUTSIDE_SPATIAL_BOUNDS;
        case EvaluatorStatus::OUTSIDE_TEMPORAL_BOUNDS:
          return IntegratorStatus::OUTSIDE_TEMPORAL_BOUNDS;
        default:
          return IntegratorStatus::FAIL;
      }
    }

  protected:
    vtkm::FloatDefault StepLength = 1.0f;
    vtkm::FloatDefault Tolerance = 0.001f;
  };

  template <typename Device>
  VTKM_CONT const ExecObject* PrepareForExecution(Device) const
  {
    this->PrepareForExecutionImpl(
      Device(), const_cast<vtkm::cont::VirtualObjectHandle<ExecObject>&>(this->ExecObjectHandle));
    return this->ExecObjectHandle.PrepareForExecution(Device());
  }

private:
  vtkm::cont::VirtualObjectHandle<ExecObject> ExecObjectHandle;

protected:
  vtkm::FloatDefault StepLength;
  vtkm::FloatDefault Tolerance =
    std::numeric_limits<vtkm::FloatDefault>::epsilon() * static_cast<vtkm::FloatDefault>(100.0f);

  VTKM_CONT virtual void PrepareForExecutionImpl(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::VirtualObjectHandle<ExecObject>& execObjectHandle) const = 0;

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
    IntegratorStatus Step(const vtkm::Vec3f& inpos,
                          vtkm::FloatDefault& time,
                          vtkm::Vec3f& outpos) const override
    {
      // If particle is out of either spatial or temporal boundary to begin with,
      // then return the corresponding status.
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
        return IntegratorStatus::OUTSIDE_SPATIAL_BOUNDS;
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
        return IntegratorStatus::OUTSIDE_TEMPORAL_BOUNDS;

      vtkm::Vec3f velocity;
      IntegratorStatus status = CheckStep(inpos, this->StepLength, time, velocity);
      if (status == IntegratorStatus::SUCCESS)
      {
        outpos = inpos + StepLength * velocity;
        time += StepLength;
      }
      else
        outpos = inpos;
      return status;
    }

    VTKM_EXEC
    IntegratorStatus SmallStep(vtkm::Vec3f& inpos,
                               vtkm::FloatDefault& time,
                               vtkm::Vec3f& outpos) const override
    {
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
      {
        outpos = inpos;
        return IntegratorStatus::OUTSIDE_SPATIAL_BOUNDS;
      }
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
      {
        outpos = inpos;
        return IntegratorStatus::OUTSIDE_TEMPORAL_BOUNDS;
      }

      //Stepping by this->StepLength goes beyond the bounds of the dataset.
      //We need to take an Euler step that goes outside of the dataset.
      //Use a binary search to find the largest step INSIDE the dataset.
      //Binary search uses a shrinking bracket of inside / outside, so when
      //we terminate, the outside value is the stepsize that will nudge
      //the particle outside the dataset.

      //The binary search will be between [0, this->StepLength]
      vtkm::FloatDefault stepShort = 0, stepLong = this->StepLength;
      vtkm::Vec3f currPos(inpos), velocity;
      vtkm::FloatDefault currTime = time;

      this->Evaluator.Evaluate(currPos, time, velocity);

      const vtkm::FloatDefault eps = 10 * vtkm::Epsilon<vtkm::FloatDefault>();
      vtkm::FloatDefault div = 1;
      for (int i = 0; i < 50; i++)
      {
        div *= 2;
        vtkm::FloatDefault stepCurr = stepShort + (this->StepLength / div);
        //See if we can step by stepCurr.
        IntegratorStatus status = this->CheckStep(inpos, stepCurr, time, velocity);
        if (status == IntegratorStatus::SUCCESS)
        {
          currPos = inpos + velocity * stepShort;
          stepShort = stepCurr;
        }
        else
          stepLong = stepCurr;

        //Stop if step bracket is small enough.
        if (stepLong - stepShort < eps)
          break;
      }

      //Take Euler step.
      time = currTime + stepShort;
      this->Evaluator.Evaluate(currPos, time, velocity);

      outpos = currPos + stepLong * velocity;
      return IntegratorStatus::OUTSIDE_SPATIAL_BOUNDS;
    }

    VTKM_EXEC
    IntegratorStatus CheckStep(const vtkm::Vec3f& inpos,
                               vtkm::FloatDefault stepLength,
                               vtkm::FloatDefault time,
                               vtkm::Vec3f& velocity) const
    {
      return static_cast<const DerivedType*>(this)->CheckStep(inpos, stepLength, time, velocity);
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
    vtkm::cont::VirtualObjectHandle<Integrator::ExecObject>& execObjectHandle,
    const EvaluatorType& evaluator,
    vtkm::FloatDefault stepLength,
    vtkm::FloatDefault tolerance) const
  {
    IntegratorType<Device>* integrator =
      new IntegratorType<Device>(evaluator.PrepareForExecution(Device()), stepLength, tolerance);
    execObjectHandle.Reset(integrator);
    return true;
  }
};

} // namespace detail

template <typename FieldEvaluateType>
class RK4Integrator : public Integrator
{
public:
  VTKM_CONT
  RK4Integrator() = default;

  VTKM_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, vtkm::FloatDefault stepLength)
    : Integrator(stepLength)
    , Evaluator(evaluator)
  {
  }

  template <typename Device>
  class ExecObject : public Integrator::ExecObjectBaseImpl<
                       decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device())),
                       typename RK4Integrator::template ExecObject<Device>>
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using FieldEvaluateExecType =
      decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device()));
    using Superclass =
      Integrator::ExecObjectBaseImpl<FieldEvaluateExecType,
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
    IntegratorStatus CheckStep(const vtkm::Vec3f& inpos,
                               vtkm::FloatDefault stepLength,
                               vtkm::FloatDefault time,
                               vtkm::Vec3f& velocity) const
    {
      vtkm::FloatDefault boundary = this->Evaluator.GetTemporalBoundary(static_cast<vtkm::Id>(1));
      if ((time + stepLength + vtkm::Epsilon<vtkm::FloatDefault>() - boundary) > 0.0)
        stepLength = boundary - time;

      vtkm::FloatDefault var1 = (stepLength / static_cast<vtkm::FloatDefault>(2));
      vtkm::FloatDefault var2 = time + var1;
      vtkm::FloatDefault var3 = time + stepLength;

      vtkm::Vec3f k1 = vtkm::TypeTraits<vtkm::Vec3f>::ZeroInitialization();
      vtkm::Vec3f k2 = k1, k3 = k1, k4 = k1;

      EvaluatorStatus status;
      status = this->Evaluator.Evaluate(inpos, time, k1);
      if (status != EvaluatorStatus::SUCCESS)
        return this->ConvertToIntegratorStatus(status);
      status = this->Evaluator.Evaluate(inpos + var1 * k1, var2, k2);
      if (status != EvaluatorStatus::SUCCESS)
        return this->ConvertToIntegratorStatus(status);
      status = this->Evaluator.Evaluate(inpos + var1 * k2, var2, k3);
      if (status != EvaluatorStatus::SUCCESS)
        return this->ConvertToIntegratorStatus(status);
      status = this->Evaluator.Evaluate(inpos + stepLength * k3, var3, k4);
      if (status != EvaluatorStatus::SUCCESS)
        return this->ConvertToIntegratorStatus(status);

      velocity = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
      return IntegratorStatus::SUCCESS;
    }
  };

private:
  FieldEvaluateType Evaluator;

protected:
  VTKM_CONT virtual void PrepareForExecutionImpl(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::VirtualObjectHandle<Integrator::ExecObject>& execObjectHandle) const override
  {
    vtkm::cont::TryExecuteOnDevice(device,
                                   detail::IntegratorPrepareForExecutionFunctor<ExecObject>(),
                                   execObjectHandle,
                                   this->Evaluator,
                                   this->StepLength,
                                   this->Tolerance);
  }
};

template <typename FieldEvaluateType>
class EulerIntegrator : public Integrator
{
public:
  EulerIntegrator() = default;

  VTKM_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, const vtkm::FloatDefault stepLength)
    : Integrator(stepLength)
    , Evaluator(evaluator)
  {
  }

  template <typename Device>
  class ExecObject : public Integrator::ExecObjectBaseImpl<
                       decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device())),
                       typename EulerIntegrator::template ExecObject<Device>>
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using FieldEvaluateExecType =
      decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device()));
    using Superclass =
      Integrator::ExecObjectBaseImpl<FieldEvaluateExecType,
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
    IntegratorStatus CheckStep(const vtkm::Vec3f& inpos,
                               vtkm::FloatDefault vtkmNotUsed(stepLength),
                               vtkm::FloatDefault time,
                               vtkm::Vec3f& velocity) const
    {
      EvaluatorStatus status = this->Evaluator.Evaluate(inpos, time, velocity);
      return this->ConvertToIntegratorStatus(status);
    }
  };

private:
  FieldEvaluateType Evaluator;

protected:
  VTKM_CONT virtual void PrepareForExecutionImpl(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::VirtualObjectHandle<Integrator::ExecObject>& execObjectHandle) const override
  {
    vtkm::cont::TryExecuteOnDevice(device,
                                   detail::IntegratorPrepareForExecutionFunctor<ExecObject>(),
                                   execObjectHandle,
                                   this->Evaluator,
                                   this->StepLength,
                                   this->Tolerance);
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
