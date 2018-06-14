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
  using PositionPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<Device>::Portal;
  using FloatPortal = typename vtkm::cont::ArrayHandle<T>::template ExecutionTypes<Device>::Portal;

public:
  VTKM_EXEC_CONT
  ParticleExecutionObject()
    : Pos()
    , Steps()
    , Status()
    , Time()
    , MaxSteps(0)
  {
  }

  VTKM_EXEC_CONT
  ParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> posArray,
                          vtkm::cont::ArrayHandle<vtkm::Id> stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id> statusArray,
                          vtkm::cont::ArrayHandle<T> timeArray,
                          vtkm::Id maxSteps)
  {
    Pos = posArray.PrepareForInPlace(Device());
    Steps = stepsArray.PrepareForInPlace(Device());
    Status = statusArray.PrepareForInPlace(Device());
    Time = timeArray.PrepareForInPlace(Device());
    MaxSteps = maxSteps;
  }

  VTKM_EXEC
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt, ParticleStatus vtkmNotUsed(status))
  {
    // Irrespective of what the advected status of the particle is,
    // we need to set the output position as the last step taken by
    // the particle, and increase the number of steps take by 1.
    Pos.Set(idx, pt);
    vtkm::Id nSteps = Steps.Get(idx);
    Steps.Set(idx, ++nSteps);

    // Check if the particle has completed the maximum steps required.
    // If yes, set it to terminated.
    if (nSteps == MaxSteps)
      SetTerminated(idx);
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
  VTKM_EXEC
  T GetTime(const vtkm::Id& idx) const { return Time.Get(idx); }
  VTKM_EXEC
  void SetTime(const vtkm::Id& idx, T time) const { Time.Set(idx, time); }

protected:
  PositionPortal Pos;
  IdPortal Steps, Status;
  FloatPortal Time;
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
      this->PosArray, this->StepsArray, this->StatusArray, this->TimeArray, this->MaxSteps);
  }

  VTKM_EXEC_CONT
  Particles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
            vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
            vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
            vtkm::cont::ArrayHandle<T>& timeArray,
            const vtkm::Id& maxSteps)
    : PosArray(posArray)
    , StepsArray(stepsArray)
    , StatusArray(statusArray)
    , TimeArray(timeArray)
    , MaxSteps(maxSteps)
  {
  }

  Particles() {}

protected:
  bool fromArray = false;

protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<ItemType, 3>> PosArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StepsArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StatusArray;
  vtkm::cont::ArrayHandle<ItemType> TimeArray;
  vtkm::Id MaxSteps;
};


template <typename T, typename Device>
class StateRecordingParticleExecutionObject
{

private:
  using IdPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::Portal;
  using IdComponentPortal =
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::template ExecutionTypes<Device>::Portal;
  using PositionPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<Device>::Portal;
  using FloatPortal = typename vtkm::cont::ArrayHandle<T>::template ExecutionTypes<Device>::Portal;

public:
  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject()
    : Pos()
    , Steps()
    , Status()
    , Time()
    , MaxSteps(0)
    , Length(0)
    , History()
    , ValidPoint()
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticleExecutionObject(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> posArray,
                                        vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> historyArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> stepsArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> statusArray,
                                        vtkm::cont::ArrayHandle<T> timeArray,
                                        vtkm::cont::ArrayHandle<vtkm::Id> validPointArray,
                                        vtkm::Id maxSteps)
  {
    Pos = posArray.PrepareForInPlace(Device());
    Steps = stepsArray.PrepareForInPlace(Device());
    Status = statusArray.PrepareForInPlace(Device());
    Time = timeArray.PrepareForInPlace(Device());
    MaxSteps = maxSteps;
    Length = maxSteps;
    vtkm::Id numPos = posArray.GetNumberOfValues();
    History = historyArray.PrepareForOutput(numPos * Length, Device());
    ValidPoint = validPointArray.PrepareForInPlace(Device());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt, ParticleStatus vtkmNotUsed(status))
  {
    // Irrespective of what the advected status of the particle is,
    // we need to set the output position as the last step taken by
    // the particle.
    Pos.Set(idx, pt);
    vtkm::Id nSteps = Steps.Get(idx);

    // Update the step for streamline storing portals.
    // This includes updating the history and the valid points.
    vtkm::Id loc = idx * Length + nSteps;
    History.Set(loc, pt);
    ValidPoint.Set(loc, 1);

    // Increase the number of steps take by 1.
    Steps.Set(idx, ++nSteps);

    // Check if the particle has completed the maximum steps required.
    // If yes, set it to terminated.
    if (nSteps == MaxSteps)
      SetTerminated(idx);
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
  VTKM_EXEC
  T GetTime(const vtkm::Id& idx) const { return Time.Get(idx); }
  VTKM_EXEC
  void SetTime(const vtkm::Id& idx, T time) const { Time.Set(idx, time); }
  vtkm::Vec<T, 3> GetHistory(const vtkm::Id& idx, const vtkm::Id& step) const
  {
    return History.Get(idx * Length + step);
  }


private:
  PositionPortal Pos;
  IdPortal Steps, Status;
  FloatPortal Time;
  vtkm::Id MaxSteps;
  vtkm::Id Length;
  PositionPortal History;
  IdPortal ValidPoint;
};

template <typename T>
class StateRecordingParticles : vtkm::cont::ExecutionObjectBase
{
private:
  using ItemType = T;

public:
  template <typename Device>
  VTKM_CONT
    vtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<ItemType, Device>
      PrepareForExecution(Device) const
  {
    return vtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<ItemType,
                                                                                   Device>(
      PosArray, HistoryArray, StepsArray, StatusArray, TimeArray, ValidPointArray, MaxSteps);
  }

  VTKM_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& historyArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          vtkm::cont::ArrayHandle<T>& timeArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validPointArray,
                          const vtkm::Id& maxSteps)
  {
    PosArray = posArray;
    HistoryArray = historyArray;
    StepsArray = stepsArray;
    StatusArray = statusArray;
    TimeArray = timeArray;
    ValidPointArray = validPointArray;
    MaxSteps = maxSteps;
  }


protected:
  vtkm::cont::ArrayHandle<vtkm::Id> StepsArray;
  vtkm::cont::ArrayHandle<vtkm::Id> StatusArray;
  vtkm::cont::ArrayHandle<ItemType> TimeArray;
  vtkm::cont::ArrayHandle<vtkm::Id> ValidPointArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<ItemType, 3>> HistoryArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<ItemType, 3>> PosArray;
  vtkm::Id MaxSteps;
};


} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Particles_h
//============================================================================
