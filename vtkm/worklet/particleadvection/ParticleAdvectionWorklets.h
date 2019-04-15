//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
#define vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h

#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
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

class ParticleAdvectWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn idx, ExecObject integrator, ExecObject integralCurve);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename IntegratorType, typename IntegralCurveType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            const IntegratorType* integrator,
                            IntegralCurveType& integralCurve) const
  {
    vtkm::Vec<ScalarType, 3> inpos = integralCurve.GetPos(idx);
    vtkm::Vec<ScalarType, 3> outpos;
    ScalarType time = integralCurve.GetTime(idx);
    ParticleStatus status;
    while (!integralCurve.Done(idx))
    {
      status = integrator->Step(inpos, time, outpos);
      // If the status is OK, we only need to check if the particle
      // has completed the maximum steps required.
      if (status == ParticleStatus::STATUS_OK)
      {
        integralCurve.TakeStep(idx, outpos, status);
        // This is to keep track of the particle's time.
        // This is what the Evaluator uses to determine if the particle
        // has exited temporal boundary.
        integralCurve.SetTime(idx, time);
        inpos = outpos;
      }
      // If the particle is at spatial or temporal  boundary, take steps to just
      // push it a little out of the boundary so that it will start advection in
      // another domain, or in another time slice. Taking small steps enables
      // reducing the error introduced at spatial or temporal boundaries.
      if (status == ParticleStatus::AT_SPATIAL_BOUNDARY ||
          status == ParticleStatus::AT_TEMPORAL_BOUNDARY)
      {
        vtkm::Id numSteps = integralCurve.GetStep(idx);
        status = integrator->PushOutOfBoundary(inpos, numSteps, time, status, outpos);
        integralCurve.TakeStep(idx, outpos, status);
        integralCurve.SetTime(idx, time);
        if (status == ParticleStatus::EXITED_SPATIAL_BOUNDARY)
          integralCurve.SetExitedSpatialBoundary(idx);
        if (status == ParticleStatus::EXITED_TEMPORAL_BOUNDARY)
          integralCurve.SetExitedTemporalBoundary(idx);
      }
      // If the particle has exited spatial boundary, set corresponding status.
      else if (status == ParticleStatus::EXITED_SPATIAL_BOUNDARY)
      {
        integralCurve.TakeStep(idx, outpos, status);
        integralCurve.SetExitedSpatialBoundary(idx);
      }
      // If the particle has exited temporal boundary, set corresponding status.
      else if (status == ParticleStatus::EXITED_TEMPORAL_BOUNDARY)
      {
        integralCurve.TakeStep(idx, outpos, status);
        integralCurve.SetExitedTemporalBoundary(idx);
      }
    }
  }
};


template <typename IntegratorType>
class ParticleAdvectionWorklet
{
public:
  VTKM_EXEC_CONT ParticleAdvectionWorklet() {}

  ~ParticleAdvectionWorklet() {}

  void Run(const IntegratorType& integrator,
           vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& seedArray,
           vtkm::Id maxSteps,
           vtkm::cont::ArrayHandle<vtkm::Id>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id>& stepsTaken,
           vtkm::cont::ArrayHandle<ScalarType>& timeArray)
  {
    using ParticleAdvectWorkletType = vtkm::worklet::particleadvection::ParticleAdvectWorklet;
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;
    using ParticleType = vtkm::worklet::particleadvection::Particles;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());
    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    ParticleType particles(seedArray, stepsTaken, statusArray, timeArray, maxSteps);

    //Invoke particle advection worklet
    ParticleWorkletDispatchType particleWorkletDispatch;
    particleWorkletDispatch.Invoke(idxArray, integrator, particles);
  }
};

namespace detail
{
class Subtract : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  Subtract() {}
  using ControlSignature = void(FieldOut, FieldIn, FieldIn);
  using ExecutionSignature = void(_1, _2, _3);
  VTKM_EXEC void operator()(vtkm::Id& res, const vtkm::Id& x, const vtkm::Id& y) const
  {
    res = x - y;
  }
};
} // namespace detail

template <typename IntegratorType>
class StreamlineWorklet
{
public:
  template <typename PointStorage, typename FieldStorage>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>, PointStorage>& pts,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>, PointStorage>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken,
           vtkm::cont::ArrayHandle<ScalarType, FieldStorage>& timeArray)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    run(positions, polyLines, statusArray, stepsTaken, timeArray);
  }

  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

private:
  void run(vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id>& status,
           vtkm::cont::ArrayHandle<vtkm::Id>& stepsTaken,
           vtkm::cont::ArrayHandle<ScalarType>& timeArray)
  {
    using ParticleWorkletDispatchType = typename vtkm::worklet::DispatcherMapField<
      vtkm::worklet::particleadvection::ParticleAdvectWorklet>;
    using StreamlineType = vtkm::worklet::particleadvection::StateRecordingParticles;

    vtkm::cont::ArrayHandle<vtkm::Id> initialStepsTaken;
    vtkm::cont::ArrayCopy(stepsTaken, initialStepsTaken);

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());

    ParticleWorkletDispatchType particleWorkletDispatch;

    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

    vtkm::cont::ArrayHandle<vtkm::Id> validPoint;
    std::vector<vtkm::Id> vpa(static_cast<std::size_t>(numSeeds * maxSteps), 0);
    validPoint = vtkm::cont::make_ArrayHandle(vpa);

    //Compact history into positions.
    vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> history;
    StreamlineType streamlines(
      seedArray, history, stepsTaken, status, timeArray, validPoint, maxSteps);

    particleWorkletDispatch.Invoke(idxArray, integrator, streamlines);
    vtkm::cont::Algorithm::CopyIf(history, validPoint, positions, IsOne());

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTakenNow;
    stepsTakenNow.Allocate(numSeeds);
    vtkm::worklet::DispatcherMapField<detail::Subtract> subtractDispatcher{ (detail::Subtract{}) };
    subtractDispatcher.Invoke(stepsTakenNow, stepsTaken, initialStepsTaken);

    //Create cells.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
    vtkm::Id connectivityLen = vtkm::cont::Algorithm::ScanExclusive(stepsTakenNow, cellIndex);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> connCount(0, 1, connectivityLen);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayCopy(connCount, connectivity);

    vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
    cellTypes.Allocate(numSeeds);
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> polyLineShape(vtkm::CELL_SHAPE_LINE, numSeeds);
    vtkm::cont::ArrayCopy(polyLineShape, cellTypes);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellCounts;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleCast(stepsTakenNow, vtkm::IdComponent()),
                          cellCounts);

    polyLines.Fill(positions.GetNumberOfValues(), cellTypes, cellCounts, connectivity);
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> seedArray;
  vtkm::Id maxSteps;
};
}
}
} // namespace vtkm::worklet::particleadvection

#endif // vtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
