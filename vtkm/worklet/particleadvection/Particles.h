//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_Particles_h
#define vtk_m_worklet_particleadvection_Particles_h

#include <vtkm/Particle.h>
#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/worklet/particleadvection/IntegratorStatus.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{
template <typename Device>
class ParticleExecutionObject
{
public:
  VTKM_EXEC_CONT
  ParticleExecutionObject()
    : Particles()
    , MaxSteps(0)
  {
  }

  ParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Particle> particleArray, vtkm::Id maxSteps)
  {
    Particles = particleArray.PrepareForInPlace(Device());
    MaxSteps = maxSteps;
  }

  VTKM_EXEC
  vtkm::Particle GetParticle(const vtkm::Id& idx) { return this->Particles.Get(idx); }

  VTKM_EXEC
  void PreStepUpdate(const vtkm::Id& vtkmNotUsed(idx)) {}

  VTKM_EXEC
  void StepUpdate(const vtkm::Id& idx, vtkm::FloatDefault time, const vtkm::Vec3f& pt)
  {
    vtkm::Particle p = this->GetParticle(idx);
    p.Pos = pt;
    p.Time = time;
    p.NumSteps++;
    this->Particles.Set(idx, p);
  }

  VTKM_EXEC
  void StatusUpdate(const vtkm::Id& idx,
                    const vtkm::worklet::particleadvection::IntegratorStatus& status,
                    vtkm::Id maxSteps)
  {
    vtkm::Particle p = this->GetParticle(idx);

    if (p.NumSteps == maxSteps)
      p.Status.SetTerminate();

    if (status.CheckFail())
      p.Status.SetFail();
    if (status.CheckSpatialBounds())
      p.Status.SetSpatialBounds();
    if (status.CheckTemporalBounds())
      p.Status.SetTemporalBounds();
    this->Particles.Set(idx, p);
  }

  VTKM_EXEC
  bool CanContinue(const vtkm::Id& idx)
  {
    vtkm::Particle p = this->GetParticle(idx);

    return (p.Status.CheckOk() && !p.Status.CheckTerminate() && !p.Status.CheckSpatialBounds() &&
            !p.Status.CheckTemporalBounds());
  }

  VTKM_EXEC
  void UpdateTookSteps(const vtkm::Id& idx, bool val)
  {
    vtkm::Particle p = this->GetParticle(idx);
    if (val)
      p.Status.SetTookAnySteps();
    else
      p.Status.ClearTookAnySteps();
    this->Particles.Set(idx, p);
  }

protected:
  using ParticlePortal =
    typename vtkm::cont::ArrayHandle<vtkm::Particle>::template ExecutionTypes<Device>::Portal;

  ParticlePortal Particles;
  vtkm::Id MaxSteps;
};

class Particles : public vtkm::cont::ExecutionObjectBase
{
public:
  template <typename Device>
  VTKM_CONT vtkm::worklet::particleadvection::ParticleExecutionObject<Device> PrepareForExecution(
    Device) const
  {
    return vtkm::worklet::particleadvection::ParticleExecutionObject<Device>(this->ParticleArray,
                                                                             this->MaxSteps);
  }

  VTKM_CONT
  Particles(vtkm::cont::ArrayHandle<vtkm::Particle>& pArray, vtkm::Id& maxSteps)
    : ParticleArray(pArray)
    , MaxSteps(maxSteps)
  {
  }

  Particles() {}

protected:
  vtkm::cont::ArrayHandle<vtkm::Particle> ParticleArray;
  vtkm::Id MaxSteps;
};


template <typename Device>
class StateRecordingParticleExecutionObject : public ParticleExecutionObject<Device>
{
public:
  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject()
    : ParticleExecutionObject<Device>()
    , History()
    , Length(0)
    , StepCount()
    , ValidPoint()
  {
  }

  StateRecordingParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Particle> pArray,
                                        vtkm::cont::ArrayHandle<vtkm::Vec3f> historyArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> validPointArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> stepCountArray,
                                        vtkm::Id maxSteps)
    : ParticleExecutionObject<Device>(pArray, maxSteps)
    , Length(maxSteps + 1)
  {
    vtkm::Id numPos = pArray.GetNumberOfValues();
    History = historyArray.PrepareForOutput(numPos * Length, Device());
    ValidPoint = validPointArray.PrepareForInPlace(Device());
    StepCount = stepCountArray.PrepareForInPlace(Device());
  }

  VTKM_EXEC
  void PreStepUpdate(const vtkm::Id& idx)
  {
    vtkm::Particle p = this->ParticleExecutionObject<Device>::GetParticle(idx);
    if (p.NumSteps == 0)
    {
      vtkm::Id loc = idx * Length;
      this->History.Set(loc, p.Pos);
      this->ValidPoint.Set(loc, 1);
      this->StepCount.Set(idx, 1);
    }
  }

  VTKM_EXEC
  void StepUpdate(const vtkm::Id& idx, vtkm::FloatDefault time, const vtkm::Vec3f& pt)
  {
    this->ParticleExecutionObject<Device>::StepUpdate(idx, time, pt);

    //local step count.
    vtkm::Id stepCount = this->StepCount.Get(idx);

    vtkm::Id loc = idx * Length + stepCount;
    this->History.Set(loc, pt);
    this->ValidPoint.Set(loc, 1);
    this->StepCount.Set(idx, stepCount + 1);
  }

protected:
  using IdPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::Portal;
  using HistoryPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Vec3f>::template ExecutionTypes<Device>::Portal;

  HistoryPortal History;
  vtkm::Id Length;
  IdPortal StepCount;
  IdPortal ValidPoint;
};

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


  template <typename Device>
  VTKM_CONT vtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<Device>
    PrepareForExecution(Device) const
  {
    return vtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<Device>(
      this->ParticleArray,
      this->HistoryArray,
      this->ValidPointArray,
      this->StepCountArray,
      this->MaxSteps);
  }
  VTKM_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Particle>& pArray, const vtkm::Id& maxSteps)
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
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Particle>& pArray,
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
  vtkm::cont::ArrayHandle<vtkm::Particle> ParticleArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StepCountArray;
  vtkm::cont::ArrayHandle<vtkm::Id> ValidPointArray;
};


} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Particles_h
//============================================================================
