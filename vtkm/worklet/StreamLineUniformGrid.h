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

#ifndef vtk_m_worklet_StreamLineUniformGrid_h
#define vtk_m_worklet_StreamLineUniformGrid_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <iostream>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <vector>
#include <unistd.h>
#define _Debug
#define _Debug_2

namespace vtkm {
namespace worklet {

namespace internal {

  // Trilinear interpolation to calculate velocity at position
  template <typename FieldType, typename PortalType>
  VTKM_EXEC_EXPORT
  vtkm::Vec<FieldType, 3> VelocityAtPosition(
                                 vtkm::Vec<FieldType, 3> pos, 
                                 const vtkm::Id &xdim, 
                                 const vtkm::Id &ydim, 
                                 const vtkm::Id &zdim, 
                                 const vtkm::Id &planesize, 
                                 const vtkm::Id &rowsize, 
                                 const PortalType &vec_data)
  {
    // Adjust initial position to be within bounding box of grid
    if (pos[0] < 0.0f)
      pos[0] = 0.0f;
    if (pos[0] > static_cast<FieldType>(xdim - 1))
      pos[0] = static_cast<FieldType>(xdim - 1);
    if (pos[1] < 0.0f)
      pos[1] = 0.0f;
    if (pos[1] > static_cast<FieldType>(ydim - 1))
      pos[1] = static_cast<FieldType>(ydim - 1);
    if (pos[2] < 0.0f)
      pos[2] = 0.0f;
    if (pos[2] > static_cast<FieldType>(zdim - 1))
      pos[2] = static_cast<FieldType>(zdim - 1);

    // Set the eight corner indices with no wraparound
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;
    idx000[0] = static_cast<vtkm::Id>(floor(pos[0]));
    idx000[1] = static_cast<vtkm::Id>(floor(pos[1]));
    idx000[2] = static_cast<vtkm::Id>(floor(pos[2]));

    idx001 = idx000; idx001[0] = (idx001[0] + 1) <= xdim - 1 ? idx001[0] + 1 : xdim - 1;
    idx010 = idx000; idx010[1] = (idx010[1] + 1) <= ydim - 1 ? idx010[1] + 1 : ydim - 1;
    idx011 = idx010; idx011[0] = (idx011[0] + 1) <= xdim - 1 ? idx011[0] + 1 : xdim - 1;
    idx100 = idx000; idx100[2] = (idx100[2] + 1) <= zdim - 1 ? idx100[2] + 1 : zdim - 1;
    idx101 = idx100; idx101[0] = (idx101[0] + 1) <= xdim - 1 ? idx101[0] + 1 : xdim - 1;
    idx110 = idx100; idx110[1] = (idx110[1] + 1) <= ydim - 1 ? idx110[1] + 1 : ydim - 1;
    idx111 = idx110; idx111[0] = (idx111[0] + 1) <= xdim - 1 ? idx111[0] + 1 : xdim - 1;

    // Get the velocity at the eight corners
    vtkm::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
    v000 = vec_data.Get(idx000[2] * planesize + idx000[1] * rowsize + idx000[0]);
    v001 = vec_data.Get(idx001[2] * planesize + idx001[1] * rowsize + idx001[0]);
    v010 = vec_data.Get(idx010[2] * planesize + idx010[1] * rowsize + idx010[0]);
    v011 = vec_data.Get(idx011[2] * planesize + idx011[1] * rowsize + idx011[0]);
    v100 = vec_data.Get(idx100[2] * planesize + idx100[1] * rowsize + idx100[0]);
    v101 = vec_data.Get(idx101[2] * planesize + idx101[1] * rowsize + idx101[0]);
    v110 = vec_data.Get(idx110[2] * planesize + idx110[1] * rowsize + idx110[0]);
    v111 = vec_data.Get(idx111[2] * planesize + idx111[1] * rowsize + idx111[0]);

    // Interpolation in X
    vtkm::Vec<FieldType, 3> v00, v01, v10, v11;
    FieldType a = pos[0] - static_cast<FieldType>(floor(pos[0]));
    v00[0] = (1.0f - a) * v000[0] + a * v001[0];
    v00[1] = (1.0f - a) * v000[1] + a * v001[1];
    v00[2] = (1.0f - a) * v000[2] + a * v001[2];

    v01[0] = (1.0f - a) * v010[0] + a * v011[0];
    v01[1] = (1.0f - a) * v010[1] + a * v011[1];
    v01[2] = (1.0f - a) * v010[2] + a * v011[2];

    v10[0] = (1.0f - a) * v100[0] + a * v101[0];
    v10[1] = (1.0f - a) * v100[1] + a * v101[1];
    v10[2] = (1.0f - a) * v100[2] + a * v101[2];

    v11[0] = (1.0f - a) * v110[0] + a * v111[0];
    v11[1] = (1.0f - a) * v110[1] + a * v111[1];
    v11[2] = (1.0f - a) * v110[2] + a * v111[2];

    // Interpolation in Y
    vtkm::Vec<FieldType, 3> v0, v1;
    a = pos[1] - static_cast<FieldType>(floor(pos[1]));
    v0[0] = (1.0f - a) * v00[0] + a * v01[0];
    v0[1] = (1.0f - a) * v00[1] + a * v01[1];
    v0[2] = (1.0f - a) * v00[2] + a * v01[2];

    v1[0] = (1.0f - a) * v10[0] + a * v11[0];
    v1[1] = (1.0f - a) * v10[1] + a * v11[1];
    v1[2] = (1.0f - a) * v10[2] + a * v11[2];

    // Interpolation in Z
    vtkm::Vec<FieldType, 3> v;
    a = pos[2] - static_cast<FieldType>(floor(pos[2]));
    v[0] = (1.0f - a) * v0[0] + v1[0];
    v[1] = (1.0f - a) * v0[1] + v1[1];
    v[2] = (1.0f - a) * v0[2] + v1[2];
    return v;
  }
}

/// \brief Compute the streamline
template <typename FieldType, typename DeviceAdapter>
class StreamLineUniformGridFilter
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::Portal FieldPortalType;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;

  class MakeStreamLines : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputSeedId);
    typedef void ExecutionSignature(_1);
    typedef _1 InputDomain;

    FieldPortalConstType field;
    FieldPortalConstType seeds;
    FieldPortalType slLists;

    const vtkm::Id xdim, ydim, zdim, maxsteps;
    const FieldType t;
    const vtkm::Id planesize, rowsize;

    VTKM_CONT_EXPORT
    MakeStreamLines(const FieldType tFactor, 
                    const vtkm::Id max_steps, 
                    const vtkm::Id dims[3], 
                    FieldPortalConstType fieldArray, 
                    FieldPortalConstType seedArray, 
                    FieldPortalType streamArray): 
                                  t(tFactor), 
                                  maxsteps(max_steps), 
                                  xdim(dims[0]), 
                                  ydim(dims[1]), 
                                  zdim(dims[2]), 
                                  planesize(dims[0] * dims[1]),
                                  rowsize(dims[0]),
                                  field(fieldArray), 
                                  seeds(seedArray), 
                                  slLists(streamArray)
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id &seedId) const
    {
      // Set location in output array to fill for this seed (forward then backward)
      vtkm::Id index = seedId * maxsteps * 2;

      // Set initial seed position for forward tracing
      vtkm::Vec<FieldType, 3> pre_pos = seeds.Get(seedId);
      vtkm::Vec<FieldType, 3> pos = seeds.Get(seedId);
      this->slLists.Set(index++, pos);

      // Forward tracing
      for (vtkm::Id i = 1; i < maxsteps; i++)
      {
        vtkm::Vec<FieldType, 3> vel, avel, bvel, cvel, dvel;
        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug
      printf("a pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("a vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          avel[d] = t * vel[d];
          pos[d] += avel[d] / 2.0f;
        }

        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug
      printf("b pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("b vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          bvel[d] = t * vel[d];
          pos[d] += bvel[d] / 2.0f;
        }

        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug
      printf("c pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("c vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          cvel[d] = t * vel[d];
          pos[d] += cvel[d] / 2.0f;
        }

        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug
      printf("d pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("d vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          dvel[d] = t * vel[d];
          pos[d] += (avel[d] + (2.0f * bvel[d]) + (2.0f * cvel[d]) + dvel[d]) / 6.0f;
        }

        if (pos[0] < 0.0f || pos[0] > xdim || 
            pos[1] < 0.0f || pos[1] > ydim || 
            pos[2] < 0.0f || pos[2] > zdim)
        {
          pos = pre_pos;
        }
        this->slLists.Set(index++, pos);
        pre_pos = pos;
      }

      // Set initial seed position for backward tracing
      pre_pos = seeds.Get(seedId);
      pos = seeds.Get(seedId);
      this->slLists.Set(index++, pos);

      // Backward tracing
      for (vtkm::Id i = 1; i < maxsteps; i++)
      {
        vtkm::Vec<FieldType, 3> vel, avel, bvel, cvel, dvel;
        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug_2
      printf("a pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("a vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          avel[d] = t * (0.0f - vel[d]);
          pos[d] += avel[d] / 2.0f;
        }

        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug_2
      printf("b pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("b vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          bvel[d] = t * (0.0f - vel[d]);
          pos[d] += bvel[d] / 2.0f;
        }

        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug_2
      printf("c pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("c vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          cvel[d] = t * (0.0f - vel[d]);
          pos[d] += cvel[d] / 2.0f;
        }

        vel = internal::VelocityAtPosition<FieldType, FieldPortalConstType>
                                          (pos, xdim, ydim, zdim, planesize, rowsize, field);
#ifdef _Debug_2
      printf("d pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("d vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif
        for (vtkm::Id d = 0; d < 3; d++)
        {
          dvel[d] = t * (0.0f - vel[d]);
          pos[d] += (avel[d] + (2.0f * bvel[d]) + (2.0f * cvel[d]) + dvel[d]) / 6.0f;
        }

        if (pos[0] < 0.0f || pos[0] > xdim || 
            pos[1] < 0.0f || pos[1] > ydim || 
            pos[2] < 0.0f || pos[2] > zdim)
        {
          pos = pre_pos;
        }
        this->slLists.Set(index++, pos);
        pre_pos = pos;
      }
    }
  };

  StreamLineUniformGridFilter(vtkm::Id* dim,
                   vtkm::Id num_seeds,
                   vtkm::Id max_steps) :
                     g_num_seeds(num_seeds),
                     g_max_steps(max_steps)
  {
    for (vtkm::Id i = 0; i < 3; i++)
      g_dim[i] = dim[i];
  }

  vtkm::Id g_dim[3];
  vtkm::Id g_num_seeds;
  vtkm::Id g_max_steps;

  void Run(const FieldType t, 
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > fieldArray,
           vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > streamArray)
  {
    // Generate random seeds for starting streamlines
    std::vector<vtkm::Vec<FieldType, 3> > seeds;

    for (vtkm::Id i = 0; i < g_num_seeds; i++)
    {
      vtkm::Vec<FieldType, 3> secretSeed;
      secretSeed[0] = static_cast<FieldType>(rand() % g_dim[0]);
      secretSeed[1] = static_cast<FieldType>(rand() % g_dim[1]);
      secretSeed[2] = static_cast<FieldType>(rand() % g_dim[2]);
      seeds.push_back(secretSeed);
printf("Seed %ld = (%f, %f, %f)\n", i, secretSeed[0], secretSeed[1], secretSeed[2]);
    }

    vtkm::cont::ArrayHandleCounting<vtkm::Id> seedIdArray(0, 1, g_num_seeds);
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > seedsArray = 
                vtkm::cont::make_ArrayHandle(&seeds[0], seeds.size());

    // Number of streams * number of steps * [forward, backward]
    vtkm::Id totalNumParticles = g_num_seeds * g_max_steps * 2;

    // Worklet to make the streamlines
    MakeStreamLines makeStreamLines(t,
                                    g_max_steps,
                                    g_dim,
                                    fieldArray.PrepareForInput(DeviceAdapter()),
                                    seedsArray.PrepareForInput(DeviceAdapter()),
                                    streamArray.PrepareForOutput(totalNumParticles, DeviceAdapter()));
    typedef typename vtkm::worklet::DispatcherMapField<MakeStreamLines> MakeStreamLinesDispatcher;
    MakeStreamLinesDispatcher makeStreamLinesDispatcher(makeStreamLines);
    makeStreamLinesDispatcher.Invoke(seedIdArray);
  }
};

}
}

#endif // vtk_m_worklet_StreamLineUniformGrid_h
