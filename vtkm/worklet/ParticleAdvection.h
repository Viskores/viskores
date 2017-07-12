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

class ParticleAdvection
{
public:
  ParticleAdvection() {}

  template <typename IntegratorType,
            typename FieldType,
            typename PointStorage,
            typename FieldStorage,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage> Run(
    const IntegratorType& it,
    const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, PointStorage>& pts,
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, FieldStorage> fieldArray,
    const vtkm::Id& nSteps,
    const vtkm::Id& particlesPerRound,
    const DeviceAdapter&)
  {
    vtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType,
                                                               FieldType,
                                                               DeviceAdapter>
      worklet;

    return worklet.Run(it, pts, fieldArray, nSteps, particlesPerRound);
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
