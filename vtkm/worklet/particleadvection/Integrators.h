//============================================================================
//  Copyrigth (c) Kitware, Inc.
//  All rigths reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyrigth notice for more information.
//============================================================================

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
    virtual ParticleStatus SmallStep(vtkm::Vec<ScalarType, 3>& inpos,
                                     ScalarType& time,
                                     vtkm::Vec<ScalarType, 3>& outpos,
                                     const ScalarType fraction) const = 0;

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
  ScalarType Tolerance = std::numeric_limits<double>::epsilon() * 100.0;
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
                             vtkm::Vec<ScalarType, 3>& outpos,
                             const ScalarType fraction) const override
    {
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
        return ParticleStatus::AT_SPATIAL_BOUNDARY;
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
        return ParticleStatus::AT_TEMPORAL_BOUNDARY;
      const vtkm::Float64 eps = 1e-6;
      bool terminate = false;
      ScalarType stepLength = this->StepLength / 2.0f;
      while (!terminate)
      {
        vtkm::Vec<ScalarType, 3> velocity;
        if (!this->Evaluator.Evaluate(inpos, time, velocity))
        {
          return ParticleStatus::STATUS_ERROR;
        }
        if (vtkm::Abs(stepLength) <= vtkm::Abs(time) * this->Tolerance)
        {
          vtkm::Vec<ScalarType, 3> direction = velocity / vtkm::Magnitude(velocity);
          vtkm::Vec<ScalarType, 3> dirStepLength;
          vtkm::Bounds bounds;
          this->Evaluator.GetSpatialBoundary(bounds);
          dirStepLength[0] = vtkm::Abs(direction[0] * eps * bounds.X.Length());
          dirStepLength[1] = vtkm::Abs(direction[1] * eps * bounds.Y.Length());
          dirStepLength[2] = vtkm::Abs(direction[2] * eps * bounds.Z.Length());
          ScalarType minLength =
            vtkm::Min(dirStepLength[0], vtkm::Min(dirStepLength[1], dirStepLength[2]));
          if (vtkm::Abs(stepLength) < minLength)
            stepLength = minLength;
          time += stepLength;
          outpos = inpos + stepLength * velocity;
          terminate = true;
          return ParticleStatus::AT_SPATIAL_BOUNDARY;
        }
        else
        {
          ParticleStatus status = this->CheckStep(inpos, stepLength, time, velocity);
          if (status == ParticleStatus::STATUS_OK)
          {
            outpos = inpos + StepLength * velocity;
            time += StepLength;
            inpos = outpos;
          }
          else if (status == ParticleStatus::AT_TEMPORAL_BOUNDARY)
          {
            return ParticleStatus::AT_TEMPORAL_BOUNDARY;
          }
        }
        stepLength /= 2.0f;
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
