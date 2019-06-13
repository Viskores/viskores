//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_Integrators_h
#define vtk_m_worklet_particleadvection_Integrators_h

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

class Integrator : public vtkm::cont::ExecutionObjectBase
{
public:
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

protected:
  VTKM_CONT
  Integrator() = default;

  VTKM_CONT
  Integrator(ScalarType stepLength, bool minimizeError = false)
    : StepLength(stepLength)
    , MinimizeError(minimizeError)
  {
  }

public:
  class ExecObject : public vtkm::VirtualObjectBase
  {
  protected:
    VTKM_EXEC_CONT
    ExecObject(const ScalarType stepLength, ScalarType tolerance, bool minimizeError)
      : StepLength(stepLength)
      , Tolerance(tolerance)
      , MinimizeError(minimizeError)
    {
    }

  public:
    VTKM_EXEC
    virtual ParticleStatus Step(const vtkm::Vec<ScalarType, 3>& inpos,
                                ScalarType& time,
                                vtkm::Vec<ScalarType, 3>& outpos) const = 0;

    VTKM_EXEC
    virtual ParticleStatus PushOutOfBoundary(vtkm::Vec<ScalarType, 3>& inpos,
                                             vtkm::Id numSteps,
                                             ScalarType& time,
                                             ParticleStatus status,
                                             vtkm::Vec<ScalarType, 3>& outpos) const = 0;

  protected:
    ScalarType StepLength = 1.0f;
    ScalarType Tolerance = 0.001f;
    bool MinimizeError = false;
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
  ScalarType StepLength;
  ScalarType Tolerance = static_cast<ScalarType>(1 / 100.0f);
  bool MinimizeError;

  VTKM_CONT virtual void PrepareForExecutionImpl(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::VirtualObjectHandle<ExecObject>& execObjectHandle) const = 0;

  template <typename FieldEvaluateType, typename DerivedType>
  class ExecObjectBaseImpl : public ExecObject
  {
  protected:
    VTKM_EXEC_CONT
    ExecObjectBaseImpl(const FieldEvaluateType& evaluator,
                       ScalarType stepLength,
                       ScalarType tolerance,
                       bool minimizeError)
      : ExecObject(stepLength, tolerance, minimizeError)
      , Evaluator(evaluator)
    {
    }

  public:
    VTKM_EXEC
    ParticleStatus Step(const vtkm::Vec<ScalarType, 3>& inpos,
                        ScalarType& time,
                        vtkm::Vec<ScalarType, 3>& outpos) const override
    {
      // If without taking the step the particle is out of either spatial
      // or temporal boundary, then return the corresponding status.
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
        return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
        return ParticleStatus::EXITED_TEMPORAL_BOUNDARY;

      vtkm::Vec<ScalarType, 3> velocity;
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
    ParticleStatus PushOutOfBoundary(vtkm::Vec<ScalarType, 3>& inpos,
                                     vtkm::Id numSteps,
                                     ScalarType& time,
                                     ParticleStatus status,
                                     vtkm::Vec<ScalarType, 3>& outpos) const override
    {
      ScalarType stepLength = StepLength;
      vtkm::Vec<ScalarType, 3> velocity = { 0.0f, 0.0f, 0.0f };
      vtkm::Vec<ScalarType, 3> currentVelocity = { 0.0f, 0.0f, 0.0f };
      CheckStep(inpos, 0.0f, time, currentVelocity);
      numSteps = numSteps == 0 ? 1 : numSteps;
      if (MinimizeError)
      {
        //Take short steps and minimize error
        ScalarType threshold = StepLength / static_cast<ScalarType>(numSteps);
        do
        {
          stepLength /= static_cast<ScalarType>(2.0);
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
        vtkm::Vec<ScalarType, 3> spatialBoundary;
        Evaluator.GetSpatialBoundary(currentVelocity, spatialBoundary);
        ScalarType hx = (vtkm::Abs(spatialBoundary[0] - inpos[0])) / vtkm::Abs(currentVelocity[0]);
        ScalarType hy = (vtkm::Abs(spatialBoundary[1] - inpos[1])) / vtkm::Abs(currentVelocity[1]);
        ScalarType hz = (vtkm::Abs(spatialBoundary[2] - inpos[2])) / vtkm::Abs(currentVelocity[2]);
        stepLength = vtkm::Min(hx, vtkm::Min(hy, hz)) + Tolerance * stepLength;
        // Calculate the final position of the particle which is supposed to be
        // out of spatial boundary.
        outpos = inpos + stepLength * currentVelocity;
        time += stepLength;
        return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
      }
      else if (status == AT_TEMPORAL_BOUNDARY)
      {
        ScalarType temporalBoundary;
        Evaluator.GetTemporalBoundary(temporalBoundary);
        ScalarType diff = temporalBoundary - time;
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
    ParticleStatus CheckStep(const vtkm::Vec<ScalarType, 3>& inpos,
                             ScalarType stepLength,
                             ScalarType time,
                             vtkm::Vec<ScalarType, 3>& velocity) const
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
    ScalarType stepLength,
    ScalarType tolerance,
    bool minimizeError) const
  {
    IntegratorType<Device>* integrator = new IntegratorType<Device>(
      evaluator.PrepareForExecution(Device()), stepLength, tolerance, minimizeError);
    execObjectHandle.Reset(integrator);
    return true;
  }
};

} // namespace detail

template <typename FieldEvaluateType>
class RK4Integrator : public Integrator
{
public:
  using ScalarType = Integrator::ScalarType;

  VTKM_CONT
  RK4Integrator() = default;

  VTKM_CONT
  RK4Integrator(const FieldEvaluateType& evaluator,
                ScalarType stepLength,
                bool minimizeError = false)
    : Integrator(stepLength, minimizeError)
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
               ScalarType stepLength,
               ScalarType tolerance,
               bool minimizeError)
      : Superclass(evaluator, stepLength, tolerance, minimizeError)
    {
    }

    VTKM_EXEC
    ParticleStatus CheckStep(const vtkm::Vec<ScalarType, 3>& inpos,
                             ScalarType stepLength,
                             ScalarType time,
                             vtkm::Vec<ScalarType, 3>& velocity) const
    {
      ScalarType var1 = (stepLength / static_cast<ScalarType>(2));
      ScalarType var2 = time + var1;
      ScalarType var3 = time + stepLength;

      vtkm::Vec<ScalarType, 3> k1 =
        vtkm::TypeTraits<vtkm::Vec<ScalarType, 3>>::ZeroInitialization();
      vtkm::Vec<ScalarType, 3> k2 = k1, k3 = k1, k4 = k1;

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
                                   this->Tolerance,
                                   this->MinimizeError);
  }
};

template <typename FieldEvaluateType>
class EulerIntegrator : public Integrator
{
public:
  using ScalarType = Integrator::ScalarType;

  EulerIntegrator() = default;

  VTKM_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator,
                  const ScalarType stepLength,
                  bool minimizeError = false)
    : Integrator(stepLength, minimizeError)
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
               ScalarType stepLength,
               ScalarType tolerance,
               bool minimizeError)
      : Superclass(evaluator, stepLength, tolerance, minimizeError)
    {
    }

    VTKM_EXEC
    ParticleStatus CheckStep(const vtkm::Vec<ScalarType, 3>& inpos,
                             ScalarType vtkmNotUsed(stepLength),
                             ScalarType time,
                             vtkm::Vec<ScalarType, 3>& velocity) const
    {
      bool result = this->Evaluator.Evaluate(inpos, time, velocity);
      if (result)
        return ParticleStatus::STATUS_OK;
      else
        return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
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
                                   this->Tolerance,
                                   this->MinimizeError);
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
