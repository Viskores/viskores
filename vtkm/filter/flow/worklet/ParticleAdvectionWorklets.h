//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_worklet_ParticleAdvectionWorklets_h
#define vtk_m_filter_flow_worklet_ParticleAdvectionWorklets_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/Particle.h>
#include <vtkm/filter/flow/worklet/Particles.h>
#include <vtkm/worklet/WorkletMapField.h>

#ifdef VTKM_CUDA
#include <vtkm/cont/cuda/internal/ScopedCudaStackSize.h>
#endif

namespace vtkm
{
namespace worklet
{
namespace flow
{

class ParticleAdvectWorklet : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_EXEC_CONT
  ParticleAdvectWorklet()
    : PushOutOfBounds(true)
  {
  }

  VTKM_EXEC_CONT
  ParticleAdvectWorklet(bool pushOutOfBounds)
    : PushOutOfBounds(pushOutOfBounds)
  {
  }

  using ControlSignature = void(FieldIn idx, ExecObject integrator, ExecObject integralCurve);
  using ExecutionSignature = void(_1 idx, _2 integrator, _3 integralCurve);
  using InputDomain = _1;

  template <typename IntegratorType, typename IntegralCurveType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            const IntegratorType& integrator,
                            IntegralCurveType& integralCurve) const
  {
    auto particle = integralCurve.GetParticle(idx);
    vtkm::FloatDefault time = particle.GetTime();
    bool tookAnySteps = false;

    //the integrator status needs to be more robust:
    // 1. you could have success AND at temporal boundary.
    // 2. could you have success AND at spatial?
    // 3. all three?
    integralCurve.PreStepUpdate(idx, particle);
    do
    {
      particle = integralCurve.GetParticle(idx);
      vtkm::Vec3f outpos;
      auto status = integrator.Step(particle, time, outpos);
      if (status.CheckOk())
      {
        integralCurve.StepUpdate(idx, particle, time, outpos);
        tookAnySteps = true;
      }

      //We can't take a step inside spatial boundary.
      //Try and take a step just past the boundary.
      else if (status.CheckSpatialBounds() && this->PushOutOfBounds)
      {
        status = integrator.SmallStep(particle, time, outpos);
        if (status.CheckOk())
        {
          integralCurve.StepUpdate(idx, particle, time, outpos);
          tookAnySteps = true;
        }
      }
      integralCurve.StatusUpdate(idx, status);
    } while (integralCurve.CanContinue(idx));

    //Mark if any steps taken
    integralCurve.UpdateTookSteps(idx, tookAnySteps);
  }

private:
  bool PushOutOfBounds;
};


template <typename IntegratorType,
          typename ParticleType,
          typename TerminationType,
          typename AnalysisType>
class ParticleAdvectionWorklet
{
public:
  VTKM_EXEC_CONT ParticleAdvectionWorklet() {}

  ~ParticleAdvectionWorklet() {}

  void Run(const IntegratorType& integrator,
           vtkm::cont::ArrayHandle<ParticleType>& particles,
           const TerminationType& termination,
           AnalysisType& analysis)
  {

    using ParticleArrayType =
      vtkm::worklet::flow::Particles<ParticleType, TerminationType, AnalysisType>;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(particles.GetNumberOfValues());
    //Create and invoke the particle advection.
    //vtkm::cont::ArrayHandleConstant<vtkm::Id> maxSteps(MaxSteps, numSeeds);
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    // TODO: The particle advection sometimes behaves incorrectly on CUDA if the stack size
    // is not changed thusly. This is concerning as the compiler should be able to determine
    // staticly the required stack depth. What is even more concerning is that the runtime
    // does not report a stack overflow. Rather, the worklet just silently reports the wrong
    // value. Until we determine the root cause, other problems may pop up.
#ifdef VTKM_CUDA
    // This worklet needs some extra space on CUDA.
    vtkm::cont::cuda::internal::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // VTKM_CUDA

    // Initialize all the pre-requisites needed to start analysis
    // It's based on the existing properties of the particles,
    // for e.g. the number of steps they've already taken
    analysis.InitializeAnalysis(particles);

    ParticleArrayType particlesObj(particles, termination, analysis);

    vtkm::worklet::flow::ParticleAdvectWorklet worklet(analysis.SupportPushOutOfBounds());

    vtkm::cont::Invoker invoker;
    invoker(worklet, idxArray, integrator, particlesObj);

    // Finalize the analysis and clear intermittant arrays.
    analysis.FinalizeAnalysis(particles);
  }
};

}
}
} // namespace vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_ParticleAdvectionWorklets_h
