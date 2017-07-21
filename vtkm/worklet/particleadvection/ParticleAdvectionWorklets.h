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

#ifndef vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
#define vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/exec/ExecutionObjectBase.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename IntegratorType, typename FieldType, typename DeviceAdapterTag>
class ParticleAdvectWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst
    FieldPortalConstType;

  typedef void ControlSignature(FieldIn<IdType> idx, ExecObject ic);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  template <typename IntegralCurveType>
  VTKM_EXEC void operator()(const vtkm::Id& idx, IntegralCurveType& ic) const
  {
    vtkm::Vec<FieldType, 3> p = ic.GetPos(idx);
    vtkm::Vec<FieldType, 3> p2;

    while (!ic.Done(idx))
    {
      if (integrator.Step(p, field, p2))
      {
        ic.TakeStep(idx, p2);
        p = p2;
      }
      else
      {
        ic.SetExitedSpatialBoundary(idx);
      }
    }
  }

  ParticleAdvectWorklet(const IntegratorType& it, const FieldPortalConstType& f)
    : integrator(it)
    , field(f)
  {
  }

  IntegratorType integrator;
  FieldPortalConstType field;
};


template <typename IntegratorType, typename FieldType, typename DeviceAdapterTag>
class ParticleAdvectionWorklet
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst
    FieldPortalConstType;
  typedef vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType,
                                                                  FieldType,
                                                                  DeviceAdapterTag>
    ParticleAdvectWorkletType;

  ParticleAdvectionWorklet() {}

  template <typename PointStorage, typename FieldStorage>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, FieldStorage> fieldArray,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    field = fieldArray.PrepareForInput(DeviceAdapterTag());
    run(statusArray, stepsTaken);
  }

  ~ParticleAdvectionWorklet() {}

private:
  template <typename FieldStorage>
  void run(vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken)
  {
    typedef typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>
      ParticleWorkletDispatchType;
    typedef vtkm::worklet::particleadvection::Particles<FieldType, DeviceAdapterTag> ParticleType;
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> DeviceAlgorithm;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());

    //Allocate status and steps arrays.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> ok(ParticleStatus::STATUS_OK, numSeeds);
    statusArray.Allocate(numSeeds);
    DeviceAlgorithm::Copy(ok, statusArray);

    vtkm::cont::ArrayHandleConstant<vtkm::Id> zero(0, numSeeds);
    stepsTaken.Allocate(numSeeds);
    DeviceAlgorithm::Copy(zero, stepsTaken);

    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    ParticleType particles(seedArray, stepsTaken, statusArray, maxSteps);

    ParticleAdvectWorkletType particleWorklet(integrator, field);
    ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);
    particleWorkletDispatch.Invoke(idxArray, particles);
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  vtkm::cont::DataSet ds;
  vtkm::Id maxSteps;
  FieldPortalConstType field;
};


template <typename IntegratorType, typename FieldType, typename DeviceAdapterTag>
class StreamlineWorklet
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst
    FieldPortalConstType;
  typedef vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType,
                                                                  FieldType,
                                                                  DeviceAdapterTag>
    ParticleAdvectWorkletType;

  StreamlineWorklet() {}

  template <typename PointStorage, typename FieldStorage>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, FieldStorage> fieldArray,
           const vtkm::Id& nSteps,
           const vtkm::Id stepsPerRound = -1,
           const vtkm::Id& particlesPerRound = -1)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    StepsPerRound = stepsPerRound;
    ParticlesPerRound = particlesPerRound;
    field = fieldArray.PrepareForInput(DeviceAdapterTag());
    run(true);
  }

  ~StreamlineWorklet() {}

private:
  void run(bool dumpOutput = false)
  {
    typedef typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>
      ParticleWorkletDispatchType;
    typedef vtkm::worklet::particleadvection::StateRecordingParticles<FieldType, DeviceAdapterTag>
      StreamlineType;
    typedef vtkm::worklet::particleadvection::StateRecordingParticlesRound<FieldType,
                                                                           DeviceAdapterTag>
      StreamlineRoundType;

    vtkm::Id totNumSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());
    bool NeedParticleRounds = false;
    if (!(ParticlesPerRound == -1 || ParticlesPerRound > totNumSeeds))
      NeedParticleRounds = true;

    ParticleAdvectWorkletType particleWorklet(integrator, field);
    ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);

    //Brute force method, or rounds.
    if (StepsPerRound == -1)
    {
      bool particlesDone = false;
      vtkm::Id particleOffset = 0;

      while (!particlesDone)
      {
        vtkm::Id num = totNumSeeds - particleOffset;
        if (num <= 0)
          break;
        if (NeedParticleRounds && num > ParticlesPerRound)
          num = ParticlesPerRound;

        std::vector<vtkm::Id> steps((size_t)num, 0), status((size_t)num, ParticleStatus::STATUS_OK);
        vtkm::cont::ArrayHandle<vtkm::Id> stepArray = vtkm::cont::make_ArrayHandle(&steps[0], num);
        vtkm::cont::ArrayHandle<vtkm::Id> statusArray =
          vtkm::cont::make_ArrayHandle(&status[0], num);
        vtkm::cont::ArrayHandleIndex idxArray(num);

        StreamlineType streamlines(seedArray, stepArray, statusArray, maxSteps);
        particleWorkletDispatch.Invoke(idxArray, streamlines);

        if (dumpOutput)
        {
          for (vtkm::Id i = 0; i < num; i++)
          {
            vtkm::Id ns = streamlines.GetStep(i);
            for (vtkm::Id j = 0; j < ns; j++)
            {
              vtkm::Vec<FieldType, 3> p = streamlines.GetHistory(i, j);
              std::cout << p[0] << " " << p[1] << " " << p[2] << std::endl;
            }
          }
        }

        particleOffset += ParticlesPerRound;
        if (!NeedParticleRounds)
          particlesDone = true;
      }
    }
    else
    {
      bool particlesDone = false;
      vtkm::Id particleOffset = 0;

      while (!particlesDone)
      {
        vtkm::Id num = totNumSeeds - particleOffset;
        if (num <= 0)
          break;
        if (NeedParticleRounds && num > ParticlesPerRound)
          num = ParticlesPerRound;

        std::vector<vtkm::Id> steps((size_t)num, 0), status((size_t)num, ParticleStatus::STATUS_OK);
        vtkm::cont::ArrayHandle<vtkm::Id> stepArray = vtkm::cont::make_ArrayHandle(&steps[0], num);
        vtkm::cont::ArrayHandle<vtkm::Id> statusArray =
          vtkm::cont::make_ArrayHandle(&status[0], num);
        vtkm::cont::ArrayHandleIndex idxArray(num);

        vtkm::Id numSteps = 0, stepOffset = 0;
        bool stepsDone = false;
        while (!stepsDone)
        {
          numSteps += StepsPerRound;
          if (numSteps >= maxSteps)
          {
            numSteps = maxSteps;
            stepsDone = true;
          }

          StreamlineRoundType streamlines(
            seedArray, stepArray, statusArray, numSteps, StepsPerRound, stepOffset, maxSteps);
          particleWorkletDispatch.Invoke(idxArray, streamlines);

          auto historyPortal = streamlines.HistoryArray.GetPortalConstControl();
          if (dumpOutput)
          {
            for (vtkm::Id i = 0; i < num; i++)
            {
              vtkm::Id ns = streamlines.GetStep(i);
              for (vtkm::Id j = stepOffset; j < ns; j++)
              {
                vtkm::Vec<FieldType, 3> p = historyPortal.Get(i * StepsPerRound + (j - stepOffset));
                std::cout << p[0] << " " << p[1] << " " << p[2] << std::endl;
              }
            }
          }
          stepOffset += StepsPerRound;
        }
        particleOffset += ParticlesPerRound;
        if (!NeedParticleRounds)
          particlesDone = true;
      }
    }
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  vtkm::cont::DataSet ds;
  vtkm::Id maxSteps;
  vtkm::Id StepsPerRound, ParticlesPerRound;
  FieldPortalConstType field;
};
}
}
}

#endif // vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
