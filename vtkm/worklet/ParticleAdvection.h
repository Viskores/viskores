//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

struct ParticleAdvectionResult
{
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

  ParticleAdvectionResult()
    : positions()
    , status()
    , stepsTaken()
    , times()
  {
  }

  ParticleAdvectionResult(const vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& pos,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& steps)
    : positions(pos)
    , status(stat)
    , stepsTaken(steps)
  {
  }

  ParticleAdvectionResult(const vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& pos,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& steps,
                          const vtkm::cont::ArrayHandle<ScalarType>& timeArray)
    : positions(pos)
    , status(stat)
    , stepsTaken(steps)
    , times(timeArray)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> positions;
  vtkm::cont::ArrayHandle<vtkm::Id> status;
  vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
  vtkm::cont::ArrayHandle<ScalarType> times;
};

class ParticleAdvection
{
public:
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

  ParticleAdvection() {}

  template <typename IntegratorType, typename FieldType, typename PointStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
                              const vtkm::Id& nSteps)
  {
    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());

    vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
    vtkm::cont::ArrayHandle<ScalarType> timeArray;

    //Allocate status and steps arrays.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> init(0, numSeeds);
    vtkm::cont::ArrayCopy(init, stepsTaken);

    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandleConstant<ScalarType> time(0, numSeeds);
    vtkm::cont::ArrayCopy(time, timeArray);

    return Run(it, pts, stepsTaken, timeArray, nSteps);
  }

  template <typename IntegratorType, typename FieldType, typename PointStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
                              vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
                              const vtkm::Id& nSteps)
  {
    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());

    vtkm::cont::ArrayHandle<ScalarType> timeArray;
    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandleConstant<ScalarType> time(0, numSeeds);
    timeArray.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(time, timeArray);

    return Run(it, pts, inputSteps, timeArray, nSteps);
  }

  template <typename IntegratorType>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& pts,
                              vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
                              vtkm::cont::ArrayHandle<ScalarType>& inputTime,
                              const vtkm::Id& nSteps)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType> worklet;

    vtkm::Id numSeeds = static_cast<vtkm::Id>(pts.GetNumberOfValues());

    vtkm::cont::ArrayHandle<vtkm::Id> status;
    //Allocate status arrays.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status);

    worklet.Run(it, pts, nSteps, status, inputSteps, inputTime);
    //Create output.
    return ParticleAdvectionResult(pts, status, inputSteps, inputTime);
  }

  template <typename IntegratorType, typename FieldType, typename PointStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
                              vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
                              vtkm::cont::ArrayHandle<ScalarType>& inputTime,
                              const vtkm::Id& nSteps)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> ptsCopy;
    vtkm::cont::ArrayCopy(pts, ptsCopy);
    return Run(it, ptsCopy, inputSteps, inputTime, nSteps);
  }
};

struct StreamlineResult
{
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;
  using VectorType = vtkm::Vec<ScalarType, 3>;

  StreamlineResult()
    : positions()
    , polyLines()
    , status()
    , stepsTaken()
    , times()
  {
  }

  StreamlineResult(const vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& pos,
                   const vtkm::cont::CellSetExplicit<>& lines,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& steps)
    : positions(pos)
    , polyLines(lines)
    , status(stat)
    , stepsTaken(steps)
  {
  }

  StreamlineResult(const vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& pos,
                   const vtkm::cont::CellSetExplicit<>& lines,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& stat,
                   const vtkm::cont::ArrayHandle<vtkm::Id>& steps,
                   const vtkm::cont::ArrayHandle<ScalarType>& timeArray)

    : positions(pos)
    , polyLines(lines)
    , status(stat)
    , stepsTaken(steps)
    , times(timeArray)
  {
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> positions;
  vtkm::cont::CellSetExplicit<> polyLines;
  vtkm::cont::ArrayHandle<vtkm::Id> status;
  vtkm::cont::ArrayHandle<vtkm::Id> stepsTaken;
  vtkm::cont::ArrayHandle<ScalarType> times;
};

class Streamline
{
public:
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

  Streamline() {}

  template <typename IntegratorType, typename FieldType, typename PointStorage>
  StreamlineResult Run(const IntegratorType& it,
                       vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& seedArray,
                       const vtkm::Id& nSteps)
  {
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    //Allocate status and steps arrays.
    vtkm::cont::ArrayHandle<vtkm::Id> status, steps;

    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status);

    vtkm::cont::ArrayHandleConstant<vtkm::Id> zero(0, numSeeds);
    vtkm::cont::ArrayCopy(zero, steps);

    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandle<ScalarType> timeArray;
    vtkm::cont::ArrayHandleConstant<ScalarType> time(0, numSeeds);
    vtkm::cont::ArrayCopy(time, timeArray);

    return Run(it, seedArray, steps, timeArray, nSteps);
  }

  template <typename IntegratorType, typename FieldType, typename PointStorage>
  StreamlineResult Run(const IntegratorType& it,
                       vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& seedArray,
                       vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
                       const vtkm::Id& nSteps)
  {
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    //Allocate and initializr status array.
    vtkm::cont::ArrayHandle<vtkm::Id> status;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status);

    //Allocate memory to store the time for temporal integration.
    vtkm::cont::ArrayHandle<ScalarType> timeArray;
    vtkm::cont::ArrayHandleConstant<ScalarType> time(0, numSeeds);
    vtkm::cont::ArrayCopy(time, timeArray);

    return Run(it, seedArray, inputSteps, timeArray, nSteps);
  }

  template <typename IntegratorType>
  StreamlineResult Run(const IntegratorType& it,
                       vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& seedArray,
                       vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
                       vtkm::cont::ArrayHandle<ScalarType>& inputTime,
                       const vtkm::Id& nSteps)
  {
    vtkm::worklet::particleadvection::StreamlineWorklet<IntegratorType> worklet;

    vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> positions;
    vtkm::cont::CellSetExplicit<> polyLines;

    //Allocate and initialize status array.
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();
    vtkm::cont::ArrayHandle<vtkm::Id> status;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    vtkm::cont::ArrayCopy(statusOK, status);

    worklet.Run(it, seedArray, nSteps, positions, polyLines, status, inputSteps, inputTime);

    return StreamlineResult(positions, polyLines, status, inputSteps, inputTime);
  }

  template <typename IntegratorType, typename FieldType, typename PointStorage>
  StreamlineResult Run(const IntegratorType& it,
                       vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& seedArray,
                       vtkm::cont::ArrayHandle<vtkm::Id>& inputSteps,
                       vtkm::cont::ArrayHandle<ScalarType>& inputTime,
                       const vtkm::Id& nSteps)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> seedCopy;
    vtkm::cont::ArrayCopy(seedArray, seedCopy);
    return Run(it, seedCopy, inputSteps, inputTime, nSteps);
  }
};
}
}

#endif // vtk_m_worklet_ParticleAdvection_h
