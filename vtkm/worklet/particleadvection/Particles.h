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
  OK = 0,
  TERMINATE = 1,
  OUT_OF_BOUNDS = 2,
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
      SetStatusTerminate(idx);
  }

  VTKM_EXEC_CONT
  void SetStatusTerminate(const vtkm::Id& idx) { status.Set(idx, TERMINATE); }
  VTKM_EXEC_CONT
  void SetStatusOutOfBounds(const vtkm::Id& idx) { status.Set(idx, OUT_OF_BOUNDS); }

  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx) { return status.Get(idx) != OK; }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetPos(const vtkm::Id& idx) const { return pos.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStep(const vtkm::Id& idx) const { return steps.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStatus(const vtkm::Id& idx) const { return status.Get(idx); }

private:
  PosPortal pos;
  IdPortal steps;
  IdPortal status;
  vtkm::Id maxSteps;
};

template <typename T, typename DeviceAdapterTag>
class StateRecordingParticles : public vtkm::exec::ExecutionObjectBase
{
private:
  typedef
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapterTag>::Portal
      IdPortal;
  typedef typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::template ExecutionTypes<
    DeviceAdapterTag>::Portal IdComponentPortal;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>::template ExecutionTypes<
    DeviceAdapterTag>::Portal PosPortal;

public:
  VTKM_EXEC_CONT
  StateRecordingParticles(const StateRecordingParticles& s)
    : pos(s.pos)
    , steps(s.steps)
    , status(s.status)
    , validPoint(s.validPoint)
    , maxSteps(s.maxSteps)
    , histSize(s.histSize)
    , history(s.history)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticles()
    : pos()
    , steps()
    , status()
    , validPoint()
    , maxSteps(0)
    , histSize(-1)
    , history()
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticles(const PosPortal& _pos,
                          const IdPortal& _steps,
                          const IdPortal& _status,
                          const IdPortal& _validPoint,
                          const vtkm::Id& _maxSteps)
    : pos(_pos)
    , steps(_steps)
    , status(_status)
    , validPoint(_validPoint)
    , maxSteps(_maxSteps)
    , histSize()
    , history()
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& historyArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validPointArray,
                          vtkm::Id& _maxSteps)
    : maxSteps(_maxSteps)
    , histSize(_maxSteps)
  {
    pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    status = statusArray.PrepareForInPlace(DeviceAdapterTag());
    validPoint = validPointArray.PrepareForInPlace(DeviceAdapterTag());
    numPos = posArray.GetNumberOfValues();
    history = historyArray.PrepareForOutput(numPos * histSize, DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  StateRecordingParticles(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& posArray,
                          vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& historyArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& stepsArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validPointArray,
                          const vtkm::Id& _maxSteps,
                          const vtkm::Id& _histSize)
    : maxSteps(_maxSteps)
    , histSize(_histSize)
  {
    pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    status = statusArray.PrepareForInPlace(DeviceAdapterTag());
    validPoint = validPointArray.PrepareForInPlace(DeviceAdapterTag());
    numPos = posArray.GetNumberOfValues();
    history = historyArray.PrepareForOutput(numPos * histSize, DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt)
  {
    vtkm::Id nSteps = steps.Get(idx);
    vtkm::Id loc = idx * histSize + nSteps;
    history.Set(loc, pt);
    nSteps = nSteps + 1;
    steps.Set(idx, nSteps);
    validPoint.Set(loc, 1);
    if (nSteps == maxSteps)
      SetStatusTerminate(idx);
  }

  VTKM_EXEC_CONT
  void SetStatusTerminate(const vtkm::Id& idx) { status.Set(idx, TERMINATE); }
  VTKM_EXEC_CONT
  void SetStatusOutOfBounds(const vtkm::Id& idx) { status.Set(idx, OUT_OF_BOUNDS); }

  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx) { return status.Get(idx) != OK; }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetPos(const vtkm::Id& idx) const { return pos.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStep(const vtkm::Id& idx) const { return steps.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStatus(const vtkm::Id& idx) const { return status.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetHistory(const vtkm::Id& idx, const vtkm::Id& step) const
  {
    return history.Get(idx * histSize + step);
  }

private:
  PosPortal pos;
  IdPortal steps;
  IdPortal status;
  IdPortal validPoint;
  vtkm::Id maxSteps, numPos, histSize;
  PosPortal history;
};

#if 0
template <typename T, typename DeviceAdapterTag>
class StateRecordingParticlesRound : public vtkm::exec::ExecutionObjectBase
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
    : pos(s.pos)
    , steps(s.steps)
    , status(s.status)
    , maxSteps(s.maxSteps)
    , numPos(s.numPos)
    , histSize(s.histSize)
    , offset(s.offset)
    , totalMaxSteps(s.totalMaxSteps)
    , history(s.history)
  {
  }

  VTKM_EXEC_CONT
  StateRecordingParticlesRound()
    : pos()
    , steps()
    , maxSteps(0)
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
    : pos(_pos)
    , steps(_steps)
    , status(_status)
    , maxSteps(_maxSteps)
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
    : maxSteps(_maxSteps)
    , histSize(_histSize)
    , offset(_offset)
    , totalMaxSteps(_totalMaxSteps)
  {
    pos = posArray.PrepareForInPlace(DeviceAdapterTag());
    steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    status = statusArray.PrepareForInPlace(DeviceAdapterTag());
    numPos = posArray.GetNumberOfValues();
    history = historyArray.PrepareForOutput(numPos * histSize, DeviceAdapterTag());
  }

  VTKM_EXEC_CONT
  void TakeStep(const vtkm::Id& idx, const vtkm::Vec<T, 3>& pt)
  {
    vtkm::Id nSteps = steps.Get(idx);
    vtkm::Id loc = idx * histSize + (nSteps - offset);
    history.Set(loc, pt);
    nSteps = nSteps + 1;
    steps.Set(idx, nSteps);
    if (nSteps == totalMaxSteps)
      SetStatusTerminate(idx);
    pos.Set(idx, pt);
  }

  VTKM_EXEC_CONT
  void SetStatusTerminate(const vtkm::Id& idx) { status.Set(idx, TERMINATE); }
  VTKM_EXEC_CONT
  void SetStatusOutOfBounds(const vtkm::Id& idx) { status.Set(idx, OUT_OF_BOUNDS); }

  VTKM_EXEC_CONT
  bool Done(const vtkm::Id& idx)
  {
    vtkm::Id nSteps = steps.Get(idx);
    return (nSteps - offset == histSize) || status.Get(idx) != OK;
  }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetPos(const vtkm::Id& idx) const { return pos.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStep(const vtkm::Id& idx) const { return steps.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Id GetStatus(const vtkm::Id& idx) const { return status.Get(idx); }
  VTKM_EXEC_CONT
  vtkm::Vec<T, 3> GetHistory(const vtkm::Id& idx, const vtkm::Id& step) const
  {
    return history.Get(idx * histSize + step);
  }

private:
  PosPortal pos;
  IdPortal steps;
  IdPortal status;
  vtkm::Id maxSteps, numPos, histSize, offset, totalMaxSteps;
  PosPortal history;

public:
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> historyArray;
};
#endif
}
}
}


#endif // vtk_m_worklet_particleadvection_Particles_h
