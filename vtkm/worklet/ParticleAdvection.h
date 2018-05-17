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
    , times()
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

  ParticleAdvectionResult(const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& pos,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& steps,
                          const vtkm::cont::ArrayHandle<FieldType>& timeArray)
    : positions(pos)
    , status(stat)
    , stepsTaken(steps)
    , times(timeArray)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> positions;
  vtkm::cont::ArrayHandle<vtkm::Id> status;
  vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
  vtkm::cont::ArrayHandle<FieldType> times;
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
    DeviceAdapter tag)
  {
    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
    vtkm::cont::ArrayHandle<FieldType> timeArray;

    //Allocate status and steps arrays.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> init(0, numSeeds);
    stepsTaken.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(init, stepsTaken, tag);

    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandleConstant<FieldType> time(0, numSeeds);
    timeArray.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(time, timeArray, tag);

    return Run(it, pts, stepsTaken, timeArray, nSteps, tag);
  }

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  ParticleAdvectionResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
    vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
    const vtkm::Id& nSteps,
    DeviceAdapter tag)
  {
    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());

    vtkm::cont::ArrayHandle<FieldType> timeArray;
    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandleConstant<FieldType> time(0, numSeeds);
    timeArray.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(time, timeArray, tag);

    return Run(it, pts, inputSteps, timeArray, nSteps, tag);
  }

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  ParticleAdvectionResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
    vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
    vtkm::cont::ArrayHandle<FieldType>& inputTime,
    const vtkm::Id& nSteps,
    DeviceAdapter tag)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType, FieldType> worklet;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());

    vtkm::cont::ArrayHandle<vtkm::Id> status;
    //Allocate status arrays.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status, tag);

    worklet.Run(it, pts, nSteps, status, inputSteps, inputTime, tag);
    //Create output.
    ParticleAdvectionResult<FieldType> res(pts, status, inputSteps, inputTime);
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
    , times()
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

  StreamlineResult(const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& pos,
                   const vtkm::cont::CellSetExplicit<>& lines,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& steps,
                   const vtkm::cont::ArrayHandle<FieldType>& timeArray)

    : positions(pos)
    , polyLines(lines)
    , status(stat)
    , stepsTaken(steps)
    , times(timeArray)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> positions;
  vtkm::cont::CellSetExplicit<> polyLines;
  vtkm::cont::ArrayHandle<vtkm::Id> status;
  vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
  vtkm::cont::ArrayHandle<FieldType> times;
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
    DeviceAdapter tag)
  {
    using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    //Allocate status and steps arrays.
    vtkm::cont::ArrayHandle<vtkm::Id> status, steps;

    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    DeviceAlgorithm::Copy(statusOK, status);

    vtkm::cont::ArrayHandleConstant<vtkm::Id> zero(0, numSeeds);
    steps.Allocate(numSeeds);
    DeviceAlgorithm::Copy(zero, steps);

    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandle<FieldType> timeArray;
    vtkm::cont::ArrayHandleConstant<FieldType> time(0, numSeeds);
    timeArray.Allocate(numSeeds);
    DeviceAlgorithm::Copy(time, timeArray);

    return Run(it, seedArray, steps, timeArray, nSteps, tag);
  }

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  StreamlineResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& seedArray,
    vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
    const vtkm::Id& nSteps,
    DeviceAdapter tag)
  {
    using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    //Allocate and initializr status array.
    vtkm::cont::ArrayHandle<vtkm::Id> status;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    DeviceAlgorithm::Copy(statusOK, status);

    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandle<FieldType> timeArray;
    vtkm::cont::ArrayHandleConstant<FieldType> time(0, numSeeds);
    timeArray.Allocate(numSeeds);
    DeviceAlgorithm::Copy(time, timeArray);

    return Run(it, seedArray, inputSteps, timeArray, nSteps, tag);
  }

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename DeviceAdapter>
  StreamlineResult<FieldType> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& seedArray,
    vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
    vtkm::cont::ArrayHandle<FieldType>& inputTime,
    const vtkm::Id& nSteps,
    DeviceAdapter tag)
  {
    using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    vtkm::worklet::particleadvection::StreamlineWorklet<IntegratorType, FieldType> worklet;

    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage> positions;
    vtkm::cont::CellSetExplicit<> polyLines;

    //Allocate and initialize status array.
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();
    vtkm::cont::ArrayHandle<vtkm::Id> status;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    DeviceAlgorithm::Copy(statusOK, status);

    worklet.Run(it, seedArray, nSteps, positions, polyLines, status, inputSteps, inputTime, tag);

    StreamlineResult<FieldType> res(positions, polyLines, status, inputSteps, inputTime);
    return res;
  }
};
}
}

#endif // vtk_m_worklet_ParticleAdvection_h
