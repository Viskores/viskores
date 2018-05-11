//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_Particles_h
#define vtk_m_worklet_particleadvection_Particles_h

class ParticleExecutionObjectType;

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

enum ParticleStatus
{
  STATUS_OK = 1,
  TERMINATED = 1 << 1,
  AT_SPATIAL_BOUNDARY = 1 << 2,
  AT_TEMPORAL_BOUNDARY = 1 << 3,
  EXITED_SPATIAL_BOUNDARY = 1 << 4,
  EXITED_TEMPORAL_BOUNDARY = 1 << 5,
  STATUS_ERROR = 1 << 6
};

template <typename T, typename Device>
class ParticleExecutionObject
{

private:
  using IdPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::Portal;
  using PosPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<Device>::Portal;

public:
  VTKM_EXEC_CONT
  ParticleExecutionObject()
    : Pos()
    , Steps()
    , Status()
    , MaxSteps(0)
  {
  }

  VTKM_EXEC_CONT
  ParticleExecutionObject(const ParticleExecutionObject& ic)
    : Pos(ic.Pos)
    , Steps(ic.Steps)
    , Status(ic.Status)
    , MaxSteps(ic.MaxSteps)
  {
  }

  VTKM_EXEC_CONT
  ParticleExecutionObject(const PosPortal& _pos,
                          const IdPortal& _steps,
                          const IdPortal& _status,
                          const vtkm::Id& _maxSteps)
    : Pos(_pos)
    , Steps(_steps)
    , Status(_status)
    , MaxSteps(_maxSteps)
  {
  }

  VTKM_EXEC_CONT
  ParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> posArray,
                          vtkm::cont::ArrayHandle<vtkm::Id> stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id> statusArray,
                          vtkm::Id _maxSteps)
    : MaxSteps(_maxSteps)
  {
    Pos = posArray.PrepareForInPlace(Device());
    Steps = stepsArray.PrepareForInPlace(Device());
    Status = statusArray.PrepareForInPlace(Device());
  }

  VTKM_EXEC
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt, ParticleStatus status)
  {
    if (status == ParticleStatus::STATUS_OK)
    {
      Pos.Set(idx, pt);
      vtkm::Id nSteps = Steps.Get(idx);
      nSteps = nSteps + 1;
      Steps.Set(idx, nSteps);
      if (nSteps == MaxSteps)
        SetTerminated(idx);
    }
    else
    {
      Pos.Set(idx, pt);
      SetExitedSpatialBoundary(idx);
    }
  }

  /* Set/Change Status */
  VTKM_EXEC
  void SetOK(const vtkm::Id& idx)
  {
    Clear(idx);
    Status.Set(idx, STATUS_OK);
  }
  VTKM_EXEC
  void SetTerminated(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, TERMINATED);
  }
  VTKM_EXEC
  void SetExitedSpatialBoundary(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, EXITED_SPATIAL_BOUNDARY);
  }
  VTKM_EXEC
  void SetExitedTemporalBoundary(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, EXITED_TEMPORAL_BOUNDARY);
  }
  VTKM_EXEC
  void SetError(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, STATUS_ERROR);
  }

  /* Check Status */
  VTKM_EXEC
  bool OK(const vtkm::Id& idx) { return CheckBit(idx, STATUS_OK); }
  VTKM_EXEC
  bool Terminated(const vtkm::Id& idx) { return CheckBit(idx, TERMINATED); }
  VTKM_EXEC
  bool ExitedSpatialBoundary(const vtkm::Id& idx) { return CheckBit(idx, EXITED_SPATIAL_BOUNDARY); }
  VTKM_EXEC
  bool ExitedTemporalBoundary(const vtkm::Id& idx)
  {
    return CheckBit(idx, EXITED_TEMPORAL_BOUNDARY);
  }
  VTKM_EXEC
  bool Error(const vtkm::Id& idx) { return CheckBit(idx, STATUS_ERROR); }
  VTKM_EXEC
  bool Integrateable(const vtkm::Id& idx)
  {
    return OK(idx) &&
      !(Terminated(idx) || ExitedSpatialBoundary(idx) || ExitedTemporalBoundary(idx));
  }
  VTKM_EXEC
  bool Done(const vtkm::Id& idx) { return !Integrateable(idx); }

  /* Bit Operations */
  VTKM_EXEC
  void Clear(const vtkm::Id& idx) { Status.Set(idx, 0); }
  VTKM_EXEC
  void SetBit(const vtkm::Id& idx, const ParticleStatus& b)
  {
    Status.Set(idx, Status.Get(idx) | b);
  }
  VTKM_EXEC
  void ClearBit(const vtkm::Id& idx, const ParticleStatus& b)
  {
    Status.Set(idx, Status.Get(idx) & ~b);
  }
  VTKM_EXEC
  bool CheckBit(const vtkm::Id& idx, const ParticleStatus& b) const
  {
    return (Status.Get(idx) & b) != 0;
  }

  VTKM_EXEC
  vtkm::Vec<T, 3> GetPos(const vtkm::Id& idx) const { return Pos.Get(idx); }
  VTKM_EXEC
  vtkm::Id GetStep(const vtkm::Id& idx) const { return Steps.Get(idx); }
  VTKM_EXEC
  vtkm::Id GetStatus(const vtkm::Id& idx) const { return Status.Get(idx); }

protected:
  PosPortal Pos;
  IdPortal Steps, Status;
  vtkm::Id MaxSteps;
};

template <typename T>
class Particles : public vtkm::cont::ExecutionObjectBase
{
private:
  using ItemType = T;

public:
  template <typename Device>
  VTKM_CONT vtkm::worklet::particleadvection::ParticleExecutionObject<ItemType, Device>
    PrepareForExecution(Device) const
  {

    return vtkm::worklet::particleadvection::ParticleExecutionObject<ItemType, Device>(
      this->PosArray, this->StepsArray, this->StatusArray, this->MaxSteps);
  }

  VTKM_CONT
  Particles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
            vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
            vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
            const vtkm::Id& _maxSteps)
    : PosArray(posArray)
    , StepsArray(stepsArray)
    , StatusArray(statusArray)
    , MaxSteps(_maxSteps)
  {
  }

  Particles() {}

protected:
  bool fromArray = false;

protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> PosArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StepsArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StatusArray;
  vtkm::Id MaxSteps;
};


template <typename T, typename Device>
class StateRecordingParticleExecutionObject
  : vtkm::worklet::particleadvection::ParticleExecutionObject<T, Device>
{

private:
  using IdPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::Portal;
  using IdComponentPortal =
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::template ExecutionTypes<Device>::Portal;
  using PosPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<Device>::Portal;

public:
  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject(const StateRecordingParticleExecutionObject& s)
    : vtkm::worklet::particleadvection::ParticleExecutionObject<T, Device>()
    , ValidPoint(s.ValidPoint)
    , History(s.History)
    , HistSize(s.HistSize)
    , Pos(s.Pos)
    , Steps(s.Steps)
    , Status(s.Status)
    , MaxSteps(s.MaxSteps)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject()
    : vtkm::worklet::particleadvection::ParticleExecutionObject<T, Device>()
    , ValidPoint()
    , History()
    , HistSize(-1)
    , Pos()
    , Steps()
    , Status()
    , MaxSteps(0)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject(const PosPortal& _pos,
                                        const IdPortal& _steps,
                                        const IdPortal& _status,
                                        const IdPortal& _validPoint,
                                        const vtkm::Id& _maxSteps)
    : Pos(_pos)
    , Steps(_steps)
    , Status(_status)
    , MaxSteps(_maxSteps)
    , ValidPoint(_validPoint)
    , History()
    , HistSize()
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> posArray,
                                        vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> historyArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> stepsArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> statusArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> validPointArray,
                                        vtkm::Id _maxSteps)
  {
    this->Pos = posArray.PrepareForInPlace(Device());
    this->Steps = stepsArray.PrepareForInPlace(Device());
    this->Status = statusArray.PrepareForInPlace(Device());
    this->ValidPoint = validPointArray.PrepareForInPlace(Device());
    this->MaxSteps = _maxSteps;
    HistSize = _maxSteps;
    vtkm::Id NumPos = posArray.GetNumberOfValues();
    History = historyArray.PrepareForOutput(NumPos * HistSize, Device());
  }
  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> posArray,
                                        vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> historyArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> stepsArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> statusArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> validPointArray,
                                        vtkm::Id _maxSteps,
                                        vtkm::Id _histSize)
  {
    this->Pos = posArray.PrepareForInPlace(Device());
    this->Steps = stepsArray.PrepareForInPlace(Device());
    this->Status = statusArray.PrepareForInPlace(Device());
    this->ValidPoint = validPointArray.PrepareForInPlace(Device());
    this->MaxSteps = _maxSteps;
    HistSize = _histSize;
    vtkm::Id NumPos = posArray.GetNumberOfValues();
    History = historyArray.PrepareForOutput(NumPos * HistSize, Device());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt, ParticleStatus status)
  {
    if (status != ParticleStatus::STATUS_OK)
      return;
    vtkm::Id nSteps = this->Steps.Get(idx);
    vtkm::Id loc = idx * HistSize + nSteps;
    this->History.Set(loc, pt);
    this->ValidPoint.Set(loc, 1);
    nSteps = nSteps + 1;
    this->Steps.Set(idx, nSteps);
    if (nSteps == this->MaxSteps)
      this->SetTerminated(idx);
  }

  vtkm::Vec<T, 3> GetHistory(const vtkm::Id& idx, const vtkm::Id& step) const
  {
    return History.Get(idx * HistSize + step);
  }
  /* Set/Change Status */
  VTKM_EXEC
  void SetOK(const vtkm::Id& idx)
  {
    Clear(idx);
    Status.Set(idx, STATUS_OK);
  }
  VTKM_EXEC
  void SetTerminated(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, TERMINATED);
  }
  VTKM_EXEC
  void SetExitedSpatialBoundary(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, EXITED_SPATIAL_BOUNDARY);
  }
  VTKM_EXEC
  void SetExitedTemporalBoundary(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, EXITED_TEMPORAL_BOUNDARY);
  }
  VTKM_EXEC
  void SetError(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, STATUS_ERROR);
  }

  /* Check Status */
  VTKM_EXEC
  bool OK(const vtkm::Id& idx) { return CheckBit(idx, STATUS_OK); }
  VTKM_EXEC
  bool Terminated(const vtkm::Id& idx) { return CheckBit(idx, TERMINATED); }
  VTKM_EXEC
  bool ExitedSpatialBoundary(const vtkm::Id& idx) { return CheckBit(idx, EXITED_SPATIAL_BOUNDARY); }
  VTKM_EXEC
  bool ExitedTemporalBoundary(const vtkm::Id& idx)
  {
    return CheckBit(idx, EXITED_TEMPORAL_BOUNDARY);
  }
  VTKM_EXEC
  bool Error(const vtkm::Id& idx) { return CheckBit(idx, STATUS_ERROR); }
  VTKM_EXEC
  bool Integrateable(const vtkm::Id& idx)
  {
    return OK(idx) &&
      !(Terminated(idx) || ExitedSpatialBoundary(idx) || ExitedTemporalBoundary(idx));
  }
  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx) { return !this->Integrateable(idx); }
  /* Bit Operations */
  VTKM_EXEC
  void Clear(const vtkm::Id& idx) { Status.Set(idx, 0); }
  VTKM_EXEC
  void SetBit(const vtkm::Id& idx, const ParticleStatus& b)
  {
    Status.Set(idx, Status.Get(idx) | b);
  }
  VTKM_EXEC
  void ClearBit(const vtkm::Id& idx, const ParticleStatus& b)
  {
    Status.Set(idx, Status.Get(idx) & ~b);
  }
  VTKM_EXEC
  bool CheckBit(const vtkm::Id& idx, const ParticleStatus& b) const
  {
    return (Status.Get(idx) & b) != 0;
  }

  VTKM_EXEC
  vtkm::Vec<T, 3> GetPos(const vtkm::Id& idx) const { return Pos.Get(idx); }
  VTKM_EXEC
  vtkm::Id GetStep(const vtkm::Id& idx) const { return Steps.Get(idx); }
  VTKM_EXEC
  vtkm::Id GetStatus(const vtkm::Id& idx) const { return Status.Get(idx); }

protected:
  IdPortal ValidPoint;
  PosPortal History;
  vtkm::Id HistSize;
  PosPortal Pos;
  IdPortal Steps, Status;
  vtkm::Id MaxSteps;
};


template <typename T>
class StateRecordingParticles : vtkm::worklet::particleadvection::Particles<T>
{
private:
  using ItemType = T;

public:
  template <typename Device>
  VTKM_CONT vtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<T, Device>
    PrepareForExecution(Device) const
  {
    return vtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<T, Device>(
      this->PosArray,
      this->HistoryArray,
      this->StepsArray,
      this->StatusArray,
      this->ValidPointArray,
      this->MaxSteps,
      this->HistSize);
  }
  VTKM_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& historyArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validPointArray,
                          const vtkm::Id& _maxSteps)
    : ValidPointArray(validPointArray)
    , HistoryArray(historyArray)
    , HistSize(_maxSteps)
  {
    this->PosArray = posArray;
    this->StepsArray = stepsArray;
    this->StatusArray = statusArray;
    this->MaxSteps = _maxSteps;
  }

  VTKM_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& historyArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validPointArray,
                          const vtkm::Id& _maxSteps,
                          vtkm::Id& _histSize)
    : HistSize(_histSize)
    , HistoryArray(historyArray)
    , ValidPointArray(validPointArray)
  {
    this->PosArray = posArray;
    this->StepsArray = stepsArray;
    this->StatusArray = statusArray;
    this->MaxSteps = _maxSteps;
  }

protected:
  vtkm::cont::ArrayHandle<vtkm::Id> StepsArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StatusArray;
  vtkm::cont::ArrayHandle<vtkm::Id> ValidPointArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> HistoryArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> PosArray;
  vtkm::Id HistSize;
  vtkm::Id MaxSteps;
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Particles_h
//============================================================================
