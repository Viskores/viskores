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
  Integrator(ScalarType stepLength)
    : StepLength(stepLength)
  {
  }

public:
  class ExecObject : public vtkm::VirtualObjectBase
  {
  protected:
    VTKM_EXEC_CONT
    ExecObject(const ScalarType stepLength, ScalarType tolerance)
      : StepLength(stepLength)
      , Tolerance(tolerance)
    {
    }

  public:
    VTKM_EXEC
    virtual ParticleStatus Step(const vtkm::Vec<ScalarType, 3>& inpos,
                                ScalarType& time,
                                vtkm::Vec<ScalarType, 3>& outpos) const = 0;

    VTKM_EXEC
    virtual ParticleStatus SmallStep(vtkm::Vec<ScalarType, 3>& inpos,
                                     ScalarType& time,
                                     vtkm::Vec<ScalarType, 3>& outpos) const = 0;

  protected:
    ScalarType StepLength = 1.0f;
    ScalarType Tolerance = 0.001f;
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
  ScalarType Tolerance = std::numeric_limits<double>::epsilon() * 100.0;

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
                       ScalarType tolerance)
      : ExecObject(stepLength, tolerance)
      , Evaluator(evaluator)
    {
    }

  public:
    VTKM_EXEC
    ParticleStatus Step(const vtkm::Vec<ScalarType, 3>& inpos,
                        ScalarType& time,
                        vtkm::Vec<ScalarType, 3>& outpos) const override
    {
      // If particle is out of either spatial or temporal boundary to begin with,
      // then return the corresponding status.
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
        return ParticleStatus::AT_SPATIAL_BOUNDARY;
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
        return ParticleStatus::AT_TEMPORAL_BOUNDARY;

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
    ParticleStatus SmallStep(vtkm::Vec<ScalarType, 3>& inpos,
                             ScalarType& time,
                             vtkm::Vec<ScalarType, 3>& outpos) const override
    {
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
        return ParticleStatus::AT_SPATIAL_BOUNDARY;
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
        return ParticleStatus::AT_TEMPORAL_BOUNDARY;
      ScalarType optimalLength = static_cast<ScalarType>(0);
      vtkm::Id iteration = static_cast<vtkm::Id>(1);
      vtkm::Id maxIterations = static_cast<vtkm::Id>(1 << 20);
      vtkm::Vec<ScalarType, 3> velocity;
      vtkm::Vec<ScalarType, 3> workpos(inpos);
      ScalarType worktime = time;
      // According to the earlier checks this call to Evaluate should return
      // the correct velocity at the current location, this is to use just in
      // case we are not able to find the optimal lenght in 20 iterations..
      this->Evaluator.Evaluate(workpos, time, velocity);
      while (iteration < maxIterations)
      {
        iteration = iteration << 1;
        ScalarType length = optimalLength + (this->StepLength / iteration);
        ParticleStatus status = this->CheckStep(inpos, length, time, velocity);
        if (status == ParticleStatus::STATUS_OK &&
            this->Evaluator.IsWithinSpatialBoundary(inpos + velocity * length))
        {
          workpos = inpos + velocity * length;
          worktime = time + length;
          optimalLength = length;
        }
      }
      this->Evaluator.Evaluate(workpos, worktime, velocity);
      // We have calculated a large enough step length to push the particle
      // using the higher order evaluator, take a step using that evaluator.
      // Take one final step, which should be an Euler step just to push the
      // particle out of the domain boundary
      vtkm::Bounds bounds = this->Evaluator.GetSpatialBoundary();
      vtkm::Vec<ScalarType, 3> direction = velocity / vtkm::Magnitude(velocity);

      const ScalarType eps = vtkm::Epsilon<ScalarType>();
      ScalarType xStepLength =
        vtkm::Abs(direction[0] * eps * static_cast<ScalarType>(bounds.X.Length()));
      ScalarType yStepLength =
        vtkm::Abs(direction[1] * eps * static_cast<ScalarType>(bounds.Y.Length()));
      ScalarType zStepLength =
        vtkm::Abs(direction[2] * eps * static_cast<ScalarType>(bounds.Z.Length()));
      ScalarType minLength = vtkm::Min(xStepLength, vtkm::Min(yStepLength, zStepLength));

      outpos = workpos + minLength * velocity;
      time = worktime + minLength;
      return ParticleStatus::AT_SPATIAL_BOUNDARY;
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
    ScalarType tolerance) const
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
  using ScalarType = Integrator::ScalarType;

  VTKM_CONT
  RK4Integrator() = default;

  VTKM_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, ScalarType stepLength)
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
    ExecObject(const FieldEvaluateExecType& evaluator, ScalarType stepLength, ScalarType tolerance)
      : Superclass(evaluator, stepLength, tolerance)
    {
    }

    VTKM_EXEC
    ParticleStatus CheckStep(const vtkm::Vec<ScalarType, 3>& inpos,
                             ScalarType stepLength,
                             ScalarType time,
                             vtkm::Vec<ScalarType, 3>& velocity) const
    {
      ScalarType boundary = this->Evaluator.GetTemporalBoundary(static_cast<vtkm::Id>(1));
      if ((time + stepLength + vtkm::Epsilon<ScalarType>() - boundary) > 0.0)
        stepLength = boundary - time;

      ScalarType var1 = (stepLength / static_cast<ScalarType>(2));
      ScalarType var2 = time + var1;
      ScalarType var3 = time + stepLength;

      vtkm::Vec<ScalarType, 3> k1 =
        vtkm::TypeTraits<vtkm::Vec<ScalarType, 3>>::ZeroInitialization();
      vtkm::Vec<ScalarType, 3> k2 = k1, k3 = k1, k4 = k1;

      ParticleStatus status;
      status = this->Evaluator.Evaluate(inpos, time, k1);
      if (status != ParticleStatus::STATUS_OK)
        return status;
      status = this->Evaluator.Evaluate(inpos + var1 * k1, var2, k2);
      if (status != ParticleStatus::STATUS_OK)
        return status;
      status = this->Evaluator.Evaluate(inpos + var1 * k2, var2, k3);
      if (status != ParticleStatus::STATUS_OK)
        return status;
      status = this->Evaluator.Evaluate(inpos + stepLength * k3, var3, k4);
      if (status != ParticleStatus::STATUS_OK)
        return status;

      velocity = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
      return ParticleStatus::STATUS_OK;
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
  using ScalarType = Integrator::ScalarType;

  EulerIntegrator() = default;

  VTKM_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, const ScalarType stepLength)
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
    ExecObject(const FieldEvaluateExecType& evaluator, ScalarType stepLength, ScalarType tolerance)
      : Superclass(evaluator, stepLength, tolerance)
    {
    }

    VTKM_EXEC
    ParticleStatus CheckStep(const vtkm::Vec<ScalarType, 3>& inpos,
                             ScalarType vtkmNotUsed(stepLength),
                             ScalarType time,
                             vtkm::Vec<ScalarType, 3>& velocity) const
    {
      return this->Evaluator.Evaluate(inpos, time, velocity);
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
