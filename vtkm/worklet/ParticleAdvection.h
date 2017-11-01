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

#ifndef vtk_m_worklet_ParticleAdvection_h
#define vtk_m_worklet_ParticleAdvection_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/particleadvection/ParticleAdvectionWorklets.h>

namespace vtkm
{
namespace worklet
{

template <typename FieldType>
struct ParticleAdvectionResult
{
  ParticleAdvectionResult()
    : positions()
    , status()
    , stepsTaken()
  {
  }

  ParticleAdvectionResult(const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& pos,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& steps)
    : positions(pos)
    , status(stat)
    , stepsTaken(steps)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> positions;
  vtkm::cont::ArrayHandle<vtkm::Id> status;
  vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
};

class ParticleAdvection
{
public:
  ParticleAdvection() {}

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  ParticleAdvectionResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
    const vtkm::Id& nSteps,
    const DeviceAdapter&)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType,
                                                               FieldType,
                                                               DeviceAdapter>
      worklet;

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken, status;
    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());
    //Allocate status and steps arrays.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> init(0, numSeeds);
    stepsTaken.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(init, stepsTaken, DeviceAdapter());

    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status, DeviceAdapter());

    worklet.Run(it, pts, nSteps, status, stepsTaken);
    //Create output.
    ParticleAdvectionResult<FieldType> res(pts, status, stepsTaken);
    return res;
  }

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  ParticleAdvectionResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
    const vtkm::cont::ArrayHandle<vtkm::Id>& stepsAlreadyTaken,
    const vtkm::Id& nSteps,
    const DeviceAdapter&)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType,
                                                               FieldType,
                                                               DeviceAdapter>
      worklet;

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken, status;
    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());
    //Allocate status and steps arrays.
    stepsTaken.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(stepsAlreadyTaken, stepsTaken, DeviceAdapter());

    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status, DeviceAdapter());

    worklet.Run(it, pts, nSteps, status, stepsTaken);
    //Create output.
    ParticleAdvectionResult<FieldType> res(pts, status, stepsTaken);
    return res;
  }
};

template <typename FieldType>
struct StreamlineResult
{
  StreamlineResult()
    : positions()
    , polyLines()
    , status()
    , stepsTaken()
  {
  }

  StreamlineResult(const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& pos,
                   const vtkm::cont::CellSetExplicit<>& lines,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& steps)
    : positions(pos)
    , polyLines(lines)
    , status(stat)
    , stepsTaken(steps)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> positions;
  vtkm::cont::CellSetExplicit<> polyLines;
  vtkm::cont::ArrayHandle<vtkm::Id> status;
  vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
};

class Streamline
{
public:
  Streamline() {}

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  StreamlineResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& seedArray,
    const vtkm::Id& nSteps,
    const DeviceAdapter&)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
    vtkm::worklet::particleadvection::StreamlineWorklet<IntegratorType, FieldType, DeviceAdapter>
      worklet;

    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage> positions;
    vtkm::cont::CellSetExplicit<> polyLines;

    //Allocate status and steps arrays.
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();
    vtkm::Id val = vtkm::worklet::particleadvection::ParticleStatus::STATUS_OK;
    vtkm::cont::ArrayHandle<vtkm::Id> status, steps;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> ok(val, numSeeds);
    status.Allocate(numSeeds);

    DeviceAlgorithm::Copy(ok, status);

    vtkm::cont::ArrayHandleConstant<vtkm::Id> zero(0, numSeeds);
    steps.Allocate(numSeeds);
    DeviceAlgorithm::Copy(zero, steps);

    worklet.Run(it, seedArray, nSteps, positions, polyLines, status, steps);

    StreamlineResult<FieldType> res(positions, polyLines, status, steps);
    return res;
  }
};
}
}

#endif // vtk_m_worklet_ParticleAdvection_h
