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

#ifndef vtk_m_worklet_ParticleAdvection_h
#define vtk_m_worklet_ParticleAdvection_h

#include <vtkm/Types.h>
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
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(init, stepsTaken);

    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(statusOK, status);

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
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(stepsAlreadyTaken, stepsTaken);

    vtkm::cont::ArrayHandleConstant<vtkm::Id> statusOK(static_cast<vtkm::Id>(1), numSeeds);
    status.Allocate(numSeeds);
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(statusOK, status);

    worklet.Run(it, pts, nSteps, status, stepsTaken);
    //Create output.
    ParticleAdvectionResult<FieldType> res(pts, status, stepsTaken);
    return res;
  }
};

class Streamline
{
public:
  Streamline() {}

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename FieldStorage,
            typename DeviceAdapter>
  void Run(const IntegratorType& it,
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
           vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, FieldStorage> fieldArray,
           const vtkm::Id& nSteps,
           const vtkm::Id& stepsPerRound,
           const vtkm::Id& particlesPerRound,
           const DeviceAdapter&)
  {
    vtkm::worklet::particleadvection::StreamlineWorklet<IntegratorType, FieldType, DeviceAdapter>
      worklet;

    worklet.Run(it, pts, fieldArray, nSteps, stepsPerRound, particlesPerRound);
  }
};
}
}

#endif // vtk_m_worklet_ParticleAdvection_h
