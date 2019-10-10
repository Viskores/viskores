//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
#define vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h

#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/Particle.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>
//#include <vtkm/worklet/particleadvection/ParticlesAOS.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{


class ParticleAdvectWorkletAOS : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn idx, ExecObject integrator, ExecObject integralCurve);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename IntegratorType, typename ParticleType>
  VTKM_EXEC void operator()(const vtkm::Id& /*idx*/,
                            const IntegratorType* /*integrator*/,
                            ParticleType& /*particle*/) const
  {
//std::cout<<__FILE__<<" "<<__LINE__<<std::endl;
#if 0
    vtkm::Vec3f inpos = particle.GetPos(idx);
    vtkm::Vec3f outpos;
    vtkm::FloatDefault time = particle.GetTime(idx);
    IntegratorStatus status;
    bool tookAnySteps = false;
    //std::cout<<"PA_"<<idx<<" "<<inpos<<std::endl;
    while (!particle.Done(idx))
    {
      status = integrator->Step(inpos, time, outpos);
      // If the status is OK, we only need to check if the particle
      // has completed the maximum steps required.
      if (status.CheckOk())
      {
        particle.TakeStep(idx, outpos, time);
        // This is to keep track of the particle's time.
        // This is what the Evaluator uses to determine if the particle
        // has exited temporal boundary.
        inpos = outpos;
        tookAnySteps = true;
      }
      // If the particle is at spatial or temporal  boundary, take steps to just
      // push it a little out of the boundary so that it will start advection in
      // another domain, or in another time slice. Taking small steps enables
      // reducing the error introduced at spatial or temporal boundaries.
      else if (status.CheckTemporalBounds())
      {
        particle.SetExitTemporalBoundary(idx);
        break;
      }
      else if (status.CheckSpatialBounds())
      {
        status = integrator->SmallStep(inpos, time, outpos);
        particle.TakeStep(idx, outpos, time);
        if (status.CheckTemporalBounds())
        {
          particle.SetExitTemporalBoundary(idx);
          break;
        }
        else if (status.CheckSpatialBounds())
        {
          particle.SetExitSpatialBoundary(idx);
          break;
        }
        else if (status.CheckFail())
        {
          particle.SetError(idx);
          break;
        }
      }
    }
    particle.SetTookAnySteps(idx, tookAnySteps);
#endif
  }
};

//for streamlines: inherit from the worklet below.
// overload take step to do something else.
//
class ParticleAdvectWorkletAOS2 : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn idx,
                                ExecObject integrator,
                                FieldInOut particles,
                                FieldIn maxSteps);
  using ExecutionSignature = void(_1 idx, _2 integrator, _3 particles, _4 maxSteps);
  using InputDomain = _1;

  VTKM_EXEC virtual void PostProcessStep(const vtkm::Id& idx, vtkm::Particle& particle) const {}

  VTKM_EXEC virtual void StepUpdate(const vtkm::Id& idx,
                                    vtkm::Particle& particle,
                                    const vtkm::Vec3f& pos,
                                    const vtkm::FloatDefault& time) const
  {
    particle.Pos = pos;
    particle.NumSteps++;
    particle.Time = time;
    this->PostProcessStep(idx, particle);
  }

  VTKM_EXEC virtual bool ExtraCanContinue(const vtkm::Id& vtkmNotUsed(idx),
                                          const vtkm::Particle& vtkmNotUsed(particle)) const
  {
    return true;
  }

  VTKM_EXEC bool CanContinue(const vtkm::Id& idx, const vtkm::Particle& particle) const
  {
    return (particle.Status.CheckOk() && !particle.Status.CheckTerminate() &&
            !particle.Status.CheckSpatialBounds() && !particle.Status.CheckTemporalBounds() &&
            this->ExtraCanContinue(idx, particle));
  }

  template <typename ParticleType>
  VTKM_EXEC void UpdateStatus(const vtkm::Id& vtkmNotUsed(idx),
                              ParticleType& particle,
                              const IntegratorStatus& iStatus,
                              const vtkm::Id& maxSteps) const
  {
    if (particle.NumSteps == maxSteps)
      particle.Status.SetTerminate();

    if (iStatus.CheckFail())
      particle.Status.SetFail();
    if (iStatus.CheckSpatialBounds())
      particle.Status.SetSpatialBounds();
    if (iStatus.CheckTemporalBounds())
      particle.Status.SetTemporalBounds();
  }

  template <typename IntegratorType, typename ParticleType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            const IntegratorType* integrator,
                            ParticleType& particle,
                            const vtkm::Id& maxSteps) const

  {
    vtkm::Vec3f inpos = particle.Pos, outpos;
    vtkm::FloatDefault time = particle.Time;
    IntegratorStatus status;
    bool tookAnySteps = false;

    //the integrator status needs to be more robust:
    // 1. you could have success AND at temporal boundary.
    // 2. could you have success AND at spatial?
    // 3. all three?

    //std::cout<<idx<<": "<<inpos<<" #"<<particle.NumSteps<<" "<<particle.Status<<std::endl;
    do
    {
      status = integrator->Step(inpos, time, outpos);
      if (status.CheckOk())
      {
        this->StepUpdate(idx, particle, outpos, time);
        tookAnySteps = true;
        inpos = outpos;
      }

      //We can't take a step inside spatial boundary.
      //Try and take a step just past the boundary.
      else if (status.CheckSpatialBounds())
      {
        IntegratorStatus status2 = integrator->SmallStep(inpos, time, outpos);
        if (status2.CheckOk())
        {
          this->StepUpdate(idx, particle, outpos, time);
          tookAnySteps = true;

          //we took a step, so use this status to consider below.
          status = status2;
        }
      }

      this->UpdateStatus(idx, particle, status, maxSteps);

    } while (this->CanContinue(idx, particle));

    //Mark if any steps taken
    if (tookAnySteps)
      particle.Status.SetTookAnySteps();
    else
      particle.Status.ClearTookAnySteps();
  }
};


class ParticleAdvectWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn idx, ExecObject integrator, ExecObject integralCurve);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename IntegratorType, typename IntegralCurveType>
  VTKM_EXEC void operator()(const vtkm::Id& /*idx*/,
                            const IntegratorType* /*integrator*/,
                            IntegralCurveType& /*integralCurve*/) const
  {
#if 0
    vtkm::Vec3f inpos = integralCurve.GetPos(idx);
    vtkm::Vec3f outpos;
    vtkm::FloatDefault time = integralCurve.GetTime(idx);
    IntegratorStatus status;
    bool tookAnySteps = false;
    while (!integralCurve.Done(idx))
    {
      status = integrator->Step(inpos, time, outpos);
      // If the status is OK, we only need to check if the particle
      // has completed the maximum steps required.
      if (status == IntegratorStatus::SUCCESS)
      {
        integralCurve.TakeStep(idx, outpos);
        // This is to keep track of the particle's time.
        // This is what the Evaluator uses to determine if the particle
        // has exited temporal boundary.
        integralCurve.SetTime(idx, time);
        inpos = outpos;
        tookAnySteps = true;
      }
      // If the particle is at spatial or temporal  boundary, take steps to just
      // push it a little out of the boundary so that it will start advection in
      // another domain, or in another time slice. Taking small steps enables
      // reducing the error introduced at spatial or temporal boundaries.
      else if (status == IntegratorStatus::OUTSIDE_TEMPORAL_BOUNDS)
      {
        integralCurve.SetExitTemporalBoundary(idx);
        break;
      }
      else if (status == IntegratorStatus::OUTSIDE_SPATIAL_BOUNDS)
      {
        status = integrator->SmallStep(inpos, time, outpos);
        integralCurve.TakeStep(idx, outpos);
        integralCurve.SetTime(idx, time);
        if (status == IntegratorStatus::OUTSIDE_TEMPORAL_BOUNDS)
        {
          integralCurve.SetExitTemporalBoundary(idx);
          break;
        }
        else if (status == IntegratorStatus::OUTSIDE_SPATIAL_BOUNDS)
        {
          integralCurve.SetExitSpatialBoundary(idx);
          break;
        }
        else if (status == IntegratorStatus::FAIL)
        {
          integralCurve.SetError(idx);
          break;
        }
      }
    }
    integralCurve.SetTookAnySteps(idx, tookAnySteps);
#endif
  }
};


template <typename IntegratorType>
class ParticleAdvectionWorklet
{
public:
  VTKM_EXEC_CONT ParticleAdvectionWorklet() {}

  ~ParticleAdvectionWorklet() {}

  void Run(const IntegratorType& integrator,
           vtkm::cont::ArrayHandle<vtkm::Vec3f>& seedArray,
           vtkm::Id maxSteps,
           vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id>& stepsTaken,
           vtkm::cont::ArrayHandle<vtkm::FloatDefault>& timeArray)
  {
    using ParticleAdvectWorkletType = vtkm::worklet::particleadvection::ParticleAdvectWorklet;
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;
    using ParticleType = vtkm::worklet::particleadvection::Particles;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());
    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    ParticleType particles(seedArray, stepsTaken, statusArray, timeArray, maxSteps);

#ifdef VTKM_CUDA
    // This worklet needs some extra space on CUDA.
    vtkm::cont::cuda::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // VTKM_CUDA

    //Invoke particle advection worklet
    ParticleWorkletDispatchType particleWorkletDispatch;
    particleWorkletDispatch.Invoke(idxArray, integrator, particles);
  }

  //AOS version
  void Run(const IntegratorType& integrator,
           vtkm::cont::ArrayHandle<vtkm::Particle>& particles,
           const vtkm::Id& MaxSteps)
  {
    using ParticleAdvectWorkletType = vtkm::worklet::particleadvection::ParticleAdvectWorkletAOS2;
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(particles.GetNumberOfValues());
    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> maxSteps(MaxSteps, numSeeds);
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

#ifdef VTKM_CUDA
    // This worklet needs some extra space on CUDA.
    vtkm::cont::cuda::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // VTKM_CUDA

    //Invoke particle advection worklet
    ParticleWorkletDispatchType particleWorkletDispatch;
    particleWorkletDispatch.Invoke(idxArray, integrator, particles, maxSteps);
  }
};

namespace detail
{
class Subtract : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  Subtract() {}
  using ControlSignature = void(FieldOut, FieldIn, FieldIn);
  using ExecutionSignature = void(_1, _2, _3);
  VTKM_EXEC void operator()(vtkm::Id& res, const vtkm::Id& x, const vtkm::Id& y) const
  {
    res = x - y;
  }
};
} // namespace detail


class StreamlineWorkletAOS : public ParticleAdvectWorkletAOS2
{
public:
  VTKM_EXEC virtual void PostProcessStep(const vtkm::Id& idx,
                                         vtkm::Particle& particle) const override
  {
    //std::cout<<"streamline:: Post process: "<<idx<<" "<<particle.Pos<<std::endl;
  }
};


template <typename IntegratorType>
class StreamlineWorklet
{
public:
  template <typename PointStorage, typename FieldStorage>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec3f, PointStorage>& pts,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Vec3f, PointStorage>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken,
           vtkm::cont::ArrayHandle<vtkm::FloatDefault, FieldStorage>& timeArray)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    run(positions, polyLines, statusArray, stepsTaken, timeArray);
  }

  template <typename PointStorage>
  void RunAOS(const IntegratorType& it,
              const vtkm::cont::ArrayHandle<vtkm::Particle, PointStorage>& particles,
              const vtkm::Id& MaxSteps)
  {
    using ParticleAdvectWorkletType = vtkm::worklet::particleadvection::StreamlineWorkletAOS;
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;


    vtkm::Id numSeeds = static_cast<vtkm::Id>(particles.GetNumberOfValues());
    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> maxSteps(MaxSteps, numSeeds);
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

#ifdef VTKM_CUDA
    // This worklet needs some extra space on CUDA.
    vtkm::cont::cuda::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // VTKM_CUDA

    //Invoke particle advection worklet
    ParticleWorkletDispatchType particleWorkletDispatch;
    particleWorkletDispatch.Invoke(idxArray, it, particles, maxSteps);
  }


  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

private:
  void run(vtkm::cont::ArrayHandle<vtkm::Vec3f>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id>& status,
           vtkm::cont::ArrayHandle<vtkm::Id>& stepsTaken,
           vtkm::cont::ArrayHandle<vtkm::FloatDefault>& timeArray)
  {
    using ParticleWorkletDispatchType = typename vtkm::worklet::DispatcherMapField<
      vtkm::worklet::particleadvection::ParticleAdvectWorklet>;
    using StreamlineType = vtkm::worklet::particleadvection::StateRecordingParticles;

#ifdef VTKM_CUDA
    // This worklet needs some extra space on CUDA.
    vtkm::cont::cuda::ScopedCudaStackSize stack(4 * 1024);
    (void)stack;
#endif // VTKM_CUDA

    vtkm::cont::ArrayHandle<vtkm::Id> initialStepsTaken;
    vtkm::cont::ArrayCopy(stepsTaken, initialStepsTaken);

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());

    ParticleWorkletDispatchType particleWorkletDispatch;

    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

    vtkm::cont::ArrayHandle<vtkm::Id> validPoint;
    std::vector<vtkm::Id> vpa(static_cast<std::size_t>(numSeeds * maxSteps), 0);
    validPoint = vtkm::cont::make_ArrayHandle(vpa);

    //Compact history into positions.
    vtkm::cont::ArrayHandle<vtkm::Vec3f> history;
    StreamlineType streamlines(
      seedArray, history, stepsTaken, status, timeArray, validPoint, maxSteps);

    particleWorkletDispatch.Invoke(idxArray, integrator, streamlines);
    vtkm::cont::Algorithm::CopyIf(history, validPoint, positions, IsOne());

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTakenNow;
    stepsTakenNow.Allocate(numSeeds);
    vtkm::worklet::DispatcherMapField<detail::Subtract> subtractDispatcher{ (detail::Subtract{}) };
    subtractDispatcher.Invoke(stepsTakenNow, stepsTaken, initialStepsTaken);

    //Create cells.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
    vtkm::Id connectivityLen = vtkm::cont::Algorithm::ScanExclusive(stepsTakenNow, cellIndex);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> connCount(0, 1, connectivityLen);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayCopy(connCount, connectivity);

    vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
    cellTypes.Allocate(numSeeds);
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> polyLineShape(vtkm::CELL_SHAPE_POLY_LINE,
                                                               numSeeds);
    vtkm::cont::ArrayCopy(polyLineShape, cellTypes);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellCounts;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleCast(stepsTakenNow, vtkm::IdComponent()),
                          cellCounts);

    polyLines.Fill(positions.GetNumberOfValues(), cellTypes, cellCounts, connectivity);
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> seedArray;
  vtkm::Id maxSteps;
};
}
}
} // namespace vtkm::worklet::particleadvection

#endif // vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
