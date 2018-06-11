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

#ifndef vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
#define vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename IntegratorType, typename FieldType>
class ParticleAdvectWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<IdType> idx, ExecObject ic);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename IntegralCurveType>
  VTKM_EXEC void operator()(const vtkm::Id& idx, IntegralCurveType& ic) const
  {
    vtkm::Vec<FieldType, 3> inpos = ic.GetPos(idx);
    vtkm::Vec<FieldType, 3> outpos;
    FieldType time = ic.GetTime(idx);
    ParticleStatus status;
    while (!ic.Done(idx))
    {
      status = integrator.Step(inpos, time, outpos);
      // If the status is OK, we only need to check if the particle
      // has completed the maximum steps required.
      if (status == ParticleStatus::STATUS_OK)
      {
        ic.TakeStep(idx, outpos, status);
        // This is to keep track of the particle's time.
        // This is what the Evaluator uses to determine if the particle
        // has exited temporal boundary.
        ic.SetTime(idx, time);
        inpos = outpos;
      }
      // If the particle is at spatial or temporal  boundary, take steps to just
      // push it a little out of the boundary so that it will start advection in
      // another domain, or in another time slice. Taking small steps enables
      // reducing the error introduced at spatial or temporal boundaries.
      if (status == ParticleStatus::AT_SPATIAL_BOUNDARY ||
          status == ParticleStatus::AT_TEMPORAL_BOUNDARY)
      {
        vtkm::Id numSteps = ic.GetStep(idx);
        status = integrator.PushOutOfBoundary(inpos, numSteps, time, status, outpos);
        ic.TakeStep(idx, outpos, status);
        ic.SetTime(idx, time);
        if (status == ParticleStatus::EXITED_SPATIAL_BOUNDARY)
          ic.SetExitedSpatialBoundary(idx);
        if (status == ParticleStatus::EXITED_TEMPORAL_BOUNDARY)
          ic.SetExitedTemporalBoundary(idx);
      }
      // If the particle has exited spatial boundary, set corresponding status.
      else if (status == ParticleStatus::EXITED_SPATIAL_BOUNDARY)
      {
        ic.TakeStep(idx, outpos, status);
        ic.SetExitedSpatialBoundary(idx);
      }
      // If the particle has exited temporal boundary, set corresponding status.
      else if (status == ParticleStatus::EXITED_TEMPORAL_BOUNDARY)
      {
        ic.TakeStep(idx, outpos, status);
        ic.SetExitedTemporalBoundary(idx);
      }
    }
  }

  ParticleAdvectWorklet(const IntegratorType& it)
    : integrator(it)
  {
  }

  IntegratorType integrator;
};


template <typename IntegratorType, typename FieldType>
class ParticleAdvectionWorklet
{
public:
  VTKM_EXEC_CONT ParticleAdvectionWorklet() {}

  template <typename PointStorage, typename FieldStorage, typename DeviceAdapterTag>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken,
           vtkm::cont::ArrayHandle<FieldType, FieldStorage>& timeArray,
           DeviceAdapterTag tag)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    run(statusArray, stepsTaken, timeArray, tag);
  }

  ~ParticleAdvectionWorklet() {}

private:
  template <typename FieldStorage, typename DeviceAdapterTag>
  void run(vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken,
           vtkm::cont::ArrayHandle<FieldType, FieldStorage>& timeArray,
           DeviceAdapterTag)
  {
    using ParticleAdvectWorkletType =
      vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType, FieldType>;
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType, DeviceAdapterTag>;
    using ParticleType = vtkm::worklet::particleadvection::Particles<FieldType>;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());
    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    ParticleType particles(seedArray, stepsTaken, statusArray, timeArray, maxSteps);

    //Invoke particle advection worklet
    ParticleAdvectWorkletType particleWorklet(integrator);
    ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);
    particleWorkletDispatch.Invoke(idxArray, particles);
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  vtkm::Id maxSteps;
};

namespace detail
{
class Subtract : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  Subtract() {}
  using ControlSignature = void(FieldOut<>, FieldIn<>, FieldIn<>);
  using ExecutionSignature = void(_1, _2, _3);
  VTKM_EXEC void operator()(vtkm::Id& res, const vtkm::Id& x, const vtkm::Id& y) const
  {
    res = x - y;
  }
};
};

template <typename IntegratorType, typename FieldType>
class StreamlineWorklet
{
public:
  VTKM_EXEC_CONT StreamlineWorklet() {}

  template <typename PointStorage, typename FieldStorage, typename DeviceAdapterTag>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken,
           vtkm::cont::ArrayHandle<FieldType, FieldStorage>& timeArray,
           DeviceAdapterTag tag)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    run(positions, polyLines, statusArray, stepsTaken, timeArray, tag);
  }

  ~StreamlineWorklet() {}

  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

private:
  template <typename DeviceAdapterTag>
  void run(vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id>& status,
           vtkm::cont::ArrayHandle<vtkm::Id>& stepsTaken,
           vtkm::cont::ArrayHandle<FieldType>& timeArray,
           DeviceAdapterTag)
  {

    using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
    using ParticleAdvectWorkletType =
      vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType, FieldType>;
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType, DeviceAdapterTag>;
    using StreamlineType = vtkm::worklet::particleadvection::StateRecordingParticles<FieldType>;

    vtkm::cont::ArrayHandle<vtkm::Id> initialStepsTaken;
    DeviceAlgorithm::Copy(stepsTaken, initialStepsTaken);

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());

    ParticleAdvectWorkletType particleWorklet(integrator);
    ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);

    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

    vtkm::cont::ArrayHandle<vtkm::Id> validPoint;
    std::vector<vtkm::Id> vpa(static_cast<std::size_t>(numSeeds * maxSteps), 0);
    validPoint = vtkm::cont::make_ArrayHandle(vpa);

    //Compact history into positions.
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> history;
    StreamlineType streamlines(
      seedArray, history, stepsTaken, status, timeArray, validPoint, maxSteps);

    particleWorkletDispatch.Invoke(idxArray, streamlines);
    DeviceAlgorithm::CopyIf(history, validPoint, positions, IsOne());

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTakenNow;
    stepsTakenNow.Allocate(numSeeds);
    vtkm::worklet::DispatcherMapField<detail::Subtract, DeviceAdapterTag>(detail::Subtract())
      .Invoke(stepsTakenNow, stepsTaken, initialStepsTaken);

    //Create cells.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
    vtkm::Id connectivityLen = DeviceAlgorithm::ScanExclusive(stepsTakenNow, cellIndex);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> connCount(0, 1, connectivityLen);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    DeviceAlgorithm::Copy(connCount, connectivity);

    vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
    cellTypes.Allocate(numSeeds);
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> polyLineShape(vtkm::CELL_SHAPE_LINE, numSeeds);
    DeviceAlgorithm::Copy(polyLineShape, cellTypes);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellCounts;
    DeviceAlgorithm::Copy(vtkm::cont::make_ArrayHandleCast(stepsTakenNow, vtkm::IdComponent()),
                          cellCounts);

    polyLines.Fill(positions.GetNumberOfValues(), cellTypes, cellCounts, connectivity);
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  vtkm::Id maxSteps;
};
}
}
}

#endif // vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
