//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_Particles_h
#define vtk_m_worklet_particleadvection_Particles_h

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/exec/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

enum ParticleStatus
{
  STATUS_OK = 0x0000,
  TERMINATED = 0x0001,
  ENCOUNTERED_SPATIAL_BOUNDARY = 0x0002,
  ENCOUNTERED_TEMPORAL_BOUNDARY = 0x0004,
  EXITED_SPATIAL_BOUNDARY = 0x0008,
  EXITED_TEMPORAL_BOUNDARY = 0x0010,
  STATUS_ERROR = 0x0020
};

template <typename T, typename DeviceAdapterTag>
class Particles : public vtkm::exec::ExecutionObjectBase
{

private:
  typedef
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapterTag>::Portal
      IdPortal;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<
    DeviceAdapterTag>::Portal PosPortal;

public:
  VTKM_EXEC_CONT
  Particles()
    : pos()
    , steps()
    , status()
    , maxSteps(0)
  {
  }

  VTKM_EXEC_CONT
  Particles(const Particles& ic)
    : pos(ic.pos)
    , steps(ic.steps)
    , status(ic.status)
    , maxSteps(ic.maxSteps)
  {
  }

  VTKM_EXEC_CONT
  Particles(const PosPortal& _pos,
            const IdPortal& _steps,
            const IdPortal& _status,
            const vtkm::Id& _maxSteps)
    : pos(_pos)
    , steps(_steps)
    , status(_status)
    , maxSteps(_maxSteps)
  {
  }

  VTKM_EXEC_CONT
  Particles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
            vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
            vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
            const vtkm::Id& _maxSteps)
    : maxSteps(_maxSteps)
  {
    pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    status = statusArray.PrepareForInPlace(DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt)
  {
    pos.Set(idx, pt);
    vtkm::Id nSteps = steps.Get(idx);
    nSteps = nSteps + 1;
    steps.Set(idx, nSteps);
    if (nSteps == maxSteps)
      SetTerminated(idx);
  }

  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx) { return !Integrateable(idx); }

  VTKM_EXEC_CONT
  void SetTerminated(const vtkm::Id& idx)
  {
    ClearBit(idx, STATUS_OK);
    SetBit(idx, TERMINATED);
  }

  VTKM_EXEC_CONT
  void SetExitedSpatialBounds(const vtkm::Id& idx) { SetBit(idx, EXITED_SPATIAL_BOUNDARY); }

  VTKM_EXEC_CONT
  bool Terminated(const vtkm::Id& idx)
  {
    return (CheckBit(idx, TERMINATED) || CheckBit(idx, STATUS_ERROR) ||
            CheckBit(idx, EXITED_SPATIAL_BOUNDARY) || CheckBit(idx, EXITED_TEMPORAL_BOUNDARY));
  }

  VTKM_EXEC_CONT
  bool Integrateable(const vtkm::Id& idx) { return !Terminated(idx); }

  void SetBit(const vtkm::Id& idx, const ParticleStatus& b)
  {
    status.Set(idx, status.Get(idx) | b);
  }

  void ClearBit(const vtkm::Id& idx, const ParticleStatus& b)
  {
    status.Set(idx, status.Get(idx) & ~b);
  }

  bool CheckBit(const vtkm::Id& idx, const ParticleStatus& b) const { return status.Get(idx) & b; }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetPos(const vtkm::Id& idx) const { return pos.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStep(const vtkm::Id& idx) const { return steps.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStatus(const vtkm::Id& idx) const { return status.Get(idx); }

protected:
  PosPortal pos;
  IdPortal steps, status;
  vtkm::Id maxSteps;
};

template <typename T, typename DeviceAdapterTag>
class StateRecordingParticles : public Particles<T, DeviceAdapterTag>
{

private:
  typedef
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapterTag>::Portal
      IdPortal;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<
    DeviceAdapterTag>::Portal PosPortal;

public:
  VTKM_EXEC_CONT
  StateRecordingParticles(const StateRecordingParticles& s)
    : /* pos(s.pos)
    , steps(s.steps)
    , status(s.status)
    , maxSteps(s.maxSteps)
    ,*/ Particles<T, DeviceAdapterTag>(s.pos, s.steps, s.status, s.maxSteps)
    , histSize(s.histSize)
    , history(s.history)
  {
    Particles<T, DeviceAdapterTag>(s.pos, s.steps, s.status, s.maxSteps);
  }

  VTKM_EXEC_CONT
  StateRecordingParticles()
    : /* pos()
    , steps()
    , status()
    , maxSteps(0)
    ,*/ Particles<T, DeviceAdapterTag>()
    , histSize(-1)
    , history()
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticles(const PosPortal& _pos,
                          const IdPortal& _steps,
                          const IdPortal& _status,
                          const vtkm::Id& _maxSteps)
    : /* pos(_pos)
    , steps(_steps)
    , status(_status)
    , maxSteps(_maxSteps)
    ,*/ Particles<T, DeviceAdapterTag>(_pos, _steps, _status, _maxSteps)
    , histSize()
    , history()
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          const vtkm::Id& _maxSteps)
  {
    this->pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    this->steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    this->status = statusArray.PrepareForInPlace(DeviceAdapterTag());
    this->maxSteps = _maxSteps;
    histSize = _maxSteps;
    numPos = posArray.GetNumberOfValues();
    history = historyArray.PrepareForOutput(numPos * histSize, DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          const vtkm::Id& _maxSteps,
                          vtkm::Id& _histSize)
    : histSize(_histSize)
  {
    this->pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    this->steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    this->status = statusArray.PrepareForInPlace(DeviceAdapterTag());
    this->maxSteps = _maxSteps;
    histSize = _histSize;
    numPos = posArray.GetNumberOfValues();
    history = historyArray.PrepareForOutput(numPos * histSize, DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt)
  {
    vtkm::Id nSteps = this->steps.Get(idx);
    vtkm::Id loc = idx * histSize + nSteps;
    history.Set(loc, pt);
    nSteps = nSteps + 1;
    this->steps.Set(idx, nSteps);
    if (nSteps == this->maxSteps)
      this->SetTerminated(idx);
  }

  vtkm::Vec<T, 3> GetHistory(const vtkm::Id& idx, const vtkm::Id& step) const
  {
    return history.Get(idx * histSize + step);
  }

  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx) { return !this->Integrateable(idx); }

private:
  /*PosPortal pos;
  IdPortal steps,status;*/
  vtkm::Id /*maxSteps,*/ numPos, histSize;
  PosPortal history;

public:
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> historyArray;
};



template <typename T, typename DeviceAdapterTag>
class StateRecordingParticlesRound : public Particles<T, DeviceAdapterTag>
{

private:
  typedef
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapterTag>::Portal
      IdPortal;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<
    DeviceAdapterTag>::Portal PosPortal;

public:
  VTKM_EXEC_CONT
  StateRecordingParticlesRound(const StateRecordingParticlesRound& s)
    : /* pos(s.pos)
    , steps(s.steps)
    , status(s.status)
    , maxSteps(s.maxSteps)
    ,*/ Particles<T, DeviceAdapterTag>(s.pos, s.steps, s.status, s.maxSteps)
    , numPos(s.numPos)
    , histSize(s.histSize)
    , offset(s.offset)
    , totalMaxSteps(s.totalMaxSteps)
    , history(s.history)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticlesRound()
    : /* pos()
    , steps()
    , maxSteps(0)
    ,*/ Particles<T, DeviceAdapterTag>()
    , histSize(-1)
    , offset(0)
    , totalMaxSteps(0)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticlesRound(const PosPortal& _pos,
                               const IdPortal& _steps,
                               const IdPortal& _status,
                               const vtkm::Id& _maxSteps,
                               const vtkm::Id& _histSize,
                               const vtkm::Id& _offset,
                               const vtkm::Id& _totalMaxSteps)
    : /* pos(_pos)
    , steps(_steps)
    , status(_status)
    , maxSteps(_maxSteps)
    ,*/ Particles<T, DeviceAdapterTag>(_pos, _steps, _status, _maxSteps)
    , histSize(_histSize)
    , offset(_offset)
    , totalMaxSteps(_totalMaxSteps)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticlesRound(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                               vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                               vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                               const vtkm::Id& _maxSteps,
                               const vtkm::Id& _histSize,
                               const vtkm::Id& _offset,
                               const vtkm::Id& _totalMaxSteps)
    : histSize(_histSize)
    , offset(_offset)
    , totalMaxSteps(_totalMaxSteps)
  {
    this->pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    this->steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    this->status = statusArray.PrepareForInPlace(DeviceAdapterTag());
    this->maxSteps = _maxSteps;
    numPos = posArray.GetNumberOfValues();
    history = historyArray.PrepareForOutput(numPos * histSize, DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt)
  {
    vtkm::Id nSteps = this->steps.Get(idx);
    vtkm::Id loc = idx * histSize + (nSteps - offset);
    history.Set(loc, pt);
    nSteps = nSteps + 1;
    this->steps.Set(idx, nSteps);
    if (nSteps == totalMaxSteps)
      this->SetTerminated(idx);
    this->pos.Set(idx, pt);
  }

  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx)
  {
    vtkm::Id nSteps = this->steps.Get(idx);
    return (nSteps - offset == histSize) || !this->Integrateable(idx);
  }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetHistory(const vtkm::Id& idx, const vtkm::Id& step) const
  {
    return history.Get(idx * histSize + step);
  }

private:
  /*PosPortal pos;
  IdPortal steps;
  IdPortal status;*/
  vtkm::Id /*maxSteps,*/ numPos, histSize, offset, totalMaxSteps;
  PosPortal history;

public:
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> historyArray;
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm


#endif // vtk_m_worklet_particleadvection_Particles_h
