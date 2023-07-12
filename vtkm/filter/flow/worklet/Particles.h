//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_worklet_Particles_h
#define vtk_m_filter_flow_worklet_Particles_h

#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/filter/flow/worklet/IntegratorStatus.h>

#include <vtkm/filter/flow/worklet/Analysis.h>
#include <vtkm/filter/flow/worklet/Termination.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename ParticleType, typename TerminationType, typename AnalysisType>
class ParticleExecutionObject
{
public:
  VTKM_EXEC_CONT
  ParticleExecutionObject()
    : Particles()
    , Termination()
    , Analysis()
  {
  }

  ParticleExecutionObject(vtkm::cont::ArrayHandle<ParticleType> particleArray,
                          const TerminationType& termination,
                          const AnalysisType& analysis,
                          vtkm::cont::DeviceAdapterId device,
                          vtkm::cont::Token& token)
    : Termination(termination)
    , Analysis(analysis)
  {
    Particles = particleArray.PrepareForInPlace(device, token);
  }

  VTKM_EXEC
  ParticleType GetParticle(const vtkm::Id& idx) { return this->Particles.Get(idx); }

  VTKM_EXEC
  void PreStepUpdate(const vtkm::Id& idx, const ParticleType& particle)
  {
    this->Analysis.PreStepAnalyze(idx, particle);
  }

  VTKM_EXEC
  void StepUpdate(const vtkm::Id& idx,
                  const ParticleType& particle,
                  vtkm::FloatDefault time,
                  const vtkm::Vec3f& pt)
  {
    ParticleType newParticle(particle);
    newParticle.SetPosition(pt);
    newParticle.SetTime(time);
    newParticle.SetNumberOfSteps(particle.GetNumberOfSteps() + 1);
    this->Analysis.Analyze(idx, particle, newParticle);
    this->Particles.Set(idx, newParticle);
  }

  VTKM_EXEC
  void StatusUpdate(const vtkm::Id& idx, const vtkm::worklet::flow::IntegratorStatus& status)
  {
    ParticleType p(this->GetParticle(idx));

    if (status.CheckFail())
      p.GetStatus().SetFail();
    if (status.CheckSpatialBounds())
      p.GetStatus().SetSpatialBounds();
    if (status.CheckTemporalBounds())
      p.GetStatus().SetTemporalBounds();
    if (status.CheckInGhostCell())
      p.GetStatus().SetInGhostCell();
    if (status.CheckZeroVelocity())
    {
      p.GetStatus().SetZeroVelocity();
      p.GetStatus().SetTerminate();
    }

    this->Particles.Set(idx, p);
  }

  VTKM_EXEC
  bool CanContinue(const vtkm::Id& idx)
  {
    ParticleType particle(this->GetParticle(idx));
    auto terminate = this->Termination.CheckTermination(particle);
    this->Particles.Set(idx, particle);
    return terminate;
  }

  VTKM_EXEC
  void UpdateTookSteps(const vtkm::Id& idx, bool val)
  {
    ParticleType p(this->GetParticle(idx));
    if (val)
      p.GetStatus().SetTookAnySteps();
    else
      p.GetStatus().ClearTookAnySteps();
    this->Particles.Set(idx, p);
  }

protected:
  using ParticlePortal = typename vtkm::cont::ArrayHandle<ParticleType>::WritePortalType;
  ParticlePortal Particles;
  TerminationType Termination;
  AnalysisType Analysis;
};

template <typename ParticleType, typename TerminationType, typename AnalysisType>
class Particles : public vtkm::cont::ExecutionObjectBase
{
protected:
  vtkm::cont::ArrayHandle<ParticleType> ParticleArray;
  TerminationType Termination;
  AnalysisType Analysis;

public:
  VTKM_CONT
  Particles() = default;

  VTKM_CONT
  Particles(vtkm::cont::ArrayHandle<ParticleType>& pArray,
            TerminationType termination,
            AnalysisType analysis)
    : ParticleArray(pArray)
    , Termination(termination)
    , Analysis(analysis)
  {
  }

  VTKM_CONT auto PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
    -> vtkm::worklet::flow::ParticleExecutionObject<
      ParticleType,
      decltype(this->Termination.PrepareForExecution(device, token)),
      decltype(this->Analysis.PrepareForExecution(device, token))>
  {
    auto termination = this->Termination.PrepareForExecution(device, token);
    auto analysis = this->Analysis.PrepareForExecution(device, token);
    return vtkm::worklet::flow::
      ParticleExecutionObject<ParticleType, decltype(termination), decltype(analysis)>(
        this->ParticleArray, termination, analysis, device, token);
  }
};

}
}
} //vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_Particles_h
//============================================================================
