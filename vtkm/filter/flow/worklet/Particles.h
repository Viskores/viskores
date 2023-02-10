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

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
class ParticleExecutionObject
{
public:
  VTKM_EXEC_CONT
  ParticleExecutionObject()
    : Particles()
    , MaxSteps(0)
  {
  }

  ParticleExecutionObject(vtkm::cont::ArrayHandle<ParticleType> particleArray,
                          vtkm::Id maxSteps,
                          vtkm::cont::DeviceAdapterId device,
                          vtkm::cont::Token& token)
  {
    Particles = particleArray.PrepareForInPlace(device, token);
    MaxSteps = maxSteps;
  }

  VTKM_EXEC
  ParticleType GetParticle(const vtkm::Id& idx) { return this->Particles.Get(idx); }

  VTKM_EXEC
  void PreStepUpdate(const vtkm::Id& vtkmNotUsed(idx)) {}

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
    this->Particles.Set(idx, newParticle);
  }

  VTKM_EXEC
  void StatusUpdate(const vtkm::Id& idx,
                    const vtkm::worklet::flow::IntegratorStatus& status,
                    vtkm::Id maxSteps)
  {
    ParticleType p(this->GetParticle(idx));

    if (p.GetNumberOfSteps() == maxSteps)
      p.GetStatus().SetTerminate();

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
  bool CanContinue(const vtkm::Id& idx) { return this->GetParticle(idx).GetStatus().CanContinue(); }

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
  vtkm::Id MaxSteps;
};

template <typename ParticleType>
class Particles : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_CONT vtkm::worklet::flow::ParticleExecutionObject<ParticleType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return vtkm::worklet::flow::ParticleExecutionObject<ParticleType>(
      this->ParticleArray, this->MaxSteps, device, token);
  }

  VTKM_CONT
  Particles(vtkm::cont::ArrayHandle<ParticleType>& pArray, vtkm::Id& maxSteps)
    : ParticleArray(pArray)
    , MaxSteps(maxSteps)
  {
  }

  Particles() {}

protected:
  vtkm::cont::ArrayHandle<ParticleType> ParticleArray;
  vtkm::Id MaxSteps;
};


template <typename ParticleType>
class StateRecordingParticleExecutionObject : public ParticleExecutionObject<ParticleType>
{
public:
  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject()
    : ParticleExecutionObject<ParticleType>()
    , History()
    , Length(0)
    , StepCount()
    , ValidPoint()
  {
  }

  StateRecordingParticleExecutionObject(vtkm::cont::ArrayHandle<ParticleType> pArray,
                                        vtkm::cont::ArrayHandle<vtkm::Vec3f> historyArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> validPointArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> stepCountArray,
                                        vtkm::Id maxSteps,
                                        vtkm::cont::DeviceAdapterId device,
                                        vtkm::cont::Token& token)
    : ParticleExecutionObject<ParticleType>(pArray, maxSteps, device, token)
    , Length(maxSteps + 1)
  {
    vtkm::Id numPos = pArray.GetNumberOfValues();
    History = historyArray.PrepareForOutput(numPos * Length, device, token);
    ValidPoint = validPointArray.PrepareForInPlace(device, token);
    StepCount = stepCountArray.PrepareForInPlace(device, token);
  }

  VTKM_EXEC
  void PreStepUpdate(const vtkm::Id& idx)
  {
    ParticleType p = this->ParticleExecutionObject<ParticleType>::GetParticle(idx);
    if (this->StepCount.Get(idx) == 0)
    {
      vtkm::Id loc = idx * Length;
      this->History.Set(loc, p.GetPosition());
      this->ValidPoint.Set(loc, 1);
      this->StepCount.Set(idx, 1);
    }
  }

  VTKM_EXEC
  void StepUpdate(const vtkm::Id& idx,
                  const ParticleType& particle,
                  vtkm::FloatDefault time,
                  const vtkm::Vec3f& pt)
  {
    this->ParticleExecutionObject<ParticleType>::StepUpdate(idx, particle, time, pt);
    //local step count.
    vtkm::Id stepCount = this->StepCount.Get(idx);
    vtkm::Id loc = idx * Length + stepCount;
    this->History.Set(loc, pt);
    this->ValidPoint.Set(loc, 1);
    this->StepCount.Set(idx, stepCount + 1);
  }

protected:
  using IdPortal = typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType;
  using HistoryPortal = typename vtkm::cont::ArrayHandle<vtkm::Vec3f>::WritePortalType;

  HistoryPortal History;
  vtkm::Id Length;
  IdPortal StepCount;
  IdPortal ValidPoint;
};

template <typename ParticleType>
class StateRecordingParticles : vtkm::cont::ExecutionObjectBase
{
public:
  //Helper functor for compacting history
  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };


  VTKM_CONT vtkm::worklet::flow::StateRecordingParticleExecutionObject<ParticleType>
  PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return vtkm::worklet::flow::StateRecordingParticleExecutionObject<ParticleType>(
      this->ParticleArray,
      this->HistoryArray,
      this->ValidPointArray,
      this->StepCountArray,
      this->MaxSteps,
      device,
      token);
  }
  VTKM_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<ParticleType>& pArray, const vtkm::Id& maxSteps)
    : MaxSteps(maxSteps)
    , ParticleArray(pArray)
  {
    vtkm::Id numParticles = static_cast<vtkm::Id>(pArray.GetNumberOfValues());

    //Create ValidPointArray initialized to zero.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> tmp(0, (this->MaxSteps + 1) * numParticles);
    vtkm::cont::ArrayCopy(tmp, this->ValidPointArray);

    //Create StepCountArray initialized to zero.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> tmp2(0, numParticles);
    vtkm::cont::ArrayCopy(tmp2, this->StepCountArray);
  }

  VTKM_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<ParticleType>& pArray,
                          vtkm::cont::ArrayHandle<vtkm::Vec3f>& historyArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validPointArray,
                          vtkm::Id& maxSteps)
  {
    ParticleArray = pArray;
    HistoryArray = historyArray;
    ValidPointArray = validPointArray;
    MaxSteps = maxSteps;
  }

  VTKM_CONT
  void GetCompactedHistory(vtkm::cont::ArrayHandle<vtkm::Vec3f>& positions)
  {
    vtkm::cont::Algorithm::CopyIf(this->HistoryArray, this->ValidPointArray, positions, IsOne());
  }

protected:
  vtkm::cont::ArrayHandle<vtkm::Vec3f> HistoryArray;
  vtkm::Id MaxSteps;
  vtkm::cont::ArrayHandle<ParticleType> ParticleArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StepCountArray;
  vtkm::cont::ArrayHandle<vtkm::Id> ValidPointArray;
};


}
}
} //vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_Particles_h
//============================================================================
