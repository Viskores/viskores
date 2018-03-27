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
#include <vtkm/cont/ExecutionObjectFactoryBase.h>

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
  typedef void ControlSignature(FieldIn<IdType> idx, ExecObject ic);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  template <typename T, typename IntegralCurveType>
  VTKM_EXEC void operator()(const T& idx, IntegralCurveType& ic) const
  {
    vtkm::Vec<FieldType, 3> inpos = ic.GetPos(idx);
    vtkm::Vec<FieldType, 3> outpos;

    while (!ic.Done(idx))
    {
      ParticleStatus status = integrator.Step(inpos, outpos);
      if (status == ParticleStatus::STATUS_OK)
      {
        ic.TakeStep(idx, outpos, status);
        inpos = outpos;
      }
      if (status == ParticleStatus::AT_SPATIAL_BOUNDARY)
      {
        vtkm::Id numSteps = ic.GetStep(idx);
        status = integrator.PushOutOfDomain(inpos, numSteps, outpos);
      }
      if (status == ParticleStatus::EXITED_SPATIAL_BOUNDARY)
      {
        ic.TakeStep(idx, outpos, status);
        ic.SetExitedSpatialBoundary(idx);
      }
    }
  }

  ParticleAdvectWorklet(const IntegratorType& it)
    : integrator(it)
  {
  }

  IntegratorType integrator;
};


template <typename IntegratorType, typename FieldType, typename DeviceAdapterTag>
class ParticleAdvectionWorklet
{
public:
  using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  using ParticleAdvectWorkletType =
    vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType,
                                                            FieldType,
                                                            DeviceAdapterTag>;

  VTKM_EXEC_CONT ParticleAdvectionWorklet() {}

  template <typename PointStorage, typename FieldStorage>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;
    run(statusArray, stepsTaken);
  }

  ~ParticleAdvectionWorklet() {}

private:
  template <typename FieldStorage>
  void run(vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken)
  {
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;
    using ParticleExecutionObjectFactoryType =
      vtkm::worklet::particleadvection::Particles<FieldType,
                                                  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>,
                                                  vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>>;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());
    //Create and invoke the particle advection.
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    ParticleExecutionObjectFactoryType particlesExecutionObjectFacotry(
      seedArray, stepsTaken, statusArray, maxSteps);

    //Invoke particle advection worklet
    ParticleAdvectWorkletType particleWorklet(integrator);
    ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);
    particleWorkletDispatch.Invoke(idxArray, particlesExecutionObjectFacotry);
  }

  IntegratorType integrator;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  vtkm::Id maxSteps;
};


template <typename IntegratorType, typename FieldType, typename DeviceAdapterTag>
class StreamlineWorklet
{
public:
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;
  using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  using FieldPortalConstType =
    typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst;
  using ParticleAdvectWorkletType =
    vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType,
                                                            FieldType,
                                                            DeviceAdapterTag>;

  VTKM_EXEC_CONT StreamlineWorklet() {}

  template <typename PointStorage, typename FieldStorage>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           const vtkm::Id& nSteps,
           vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& statusArray,
           vtkm::cont::ArrayHandle<vtkm::Id, FieldStorage>& stepsTaken)
  {
    integrator = it;
    seedArray = pts;
    maxSteps = nSteps;

    run(positions, polyLines, statusArray, stepsTaken);
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
  void run(vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& positions,
           vtkm::cont::CellSetExplicit<>& polyLines,
           vtkm::cont::ArrayHandle<vtkm::Id>& status,
           vtkm::cont::ArrayHandle<vtkm::Id>& stepsTaken)
  {
    using ParticleWorkletDispatchType =
      typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;
    using StreamlineType = vtkm::worklet::particleadvection::StateRecordingParticles<
      FieldType,
      vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>,
      vtkm::cont::ArrayHandle<vtkm::Id>>;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(seedArray.GetNumberOfValues());

    ParticleAdvectWorkletType particleWorklet(integrator);
    ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);

    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

    vtkm::cont::ArrayHandle<vtkm::Id> validPoint;
    std::vector<vtkm::Id> vpa(static_cast<std::size_t>(numSeeds * maxSteps), 0);
    validPoint = vtkm::cont::make_ArrayHandle(vpa);

    //Compact history into positions.
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> history;
    StreamlineType streamlines(seedArray, history, stepsTaken, status, validPoint, maxSteps);

    particleWorkletDispatch.Invoke(idxArray, streamlines);
    DeviceAlgorithm::CopyIf(history, validPoint, positions, IsOne());

    //Create cells.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
    vtkm::Id connectivityLen = DeviceAlgorithm::ScanExclusive(stepsTaken, cellIndex);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> connCount(0, 1, connectivityLen);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    DeviceAlgorithm::Copy(connCount, connectivity);

    vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
    cellTypes.Allocate(numSeeds);
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> polyLineShape(vtkm::CELL_SHAPE_LINE, numSeeds);
    DeviceAlgorithm::Copy(polyLineShape, cellTypes);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellCounts;
    DeviceAlgorithm::Copy(vtkm::cont::make_ArrayHandleCast(stepsTaken, vtkm::IdComponent()),
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
