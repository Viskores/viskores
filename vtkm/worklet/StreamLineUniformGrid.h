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
//#define _Debug
//#define _Debug_2

namespace vtkm {
namespace worklet {

namespace internal {
  template <typename FieldType, typename PortalType>
  VTKM_EXEC_EXPORT
  vtkm::Vec<FieldType, 3> GetVel(vtkm::Id3 index, 
                                 const vtkm::Id &xdim, 
                                 const vtkm::Id &ydim, 
                                 const vtkm::Id &zdim, 
                                 const PortalType &vec_data)
  {
    vtkm::Id idx = index[2] * ydim * xdim + index[1] * xdim + index[0];
    return vec_data.Get(idx);
  }

  //get the velocity at position(x,y,z), need trilinear interpolation
  //
  template <typename FieldType, typename PortalType>
  VTKM_EXEC_EXPORT
  vtkm::Vec<FieldType, 3> AtPhys(vtkm::Vec<FieldType, 3> pos, 
                                 const vtkm::Id &xdim, 
                                 const vtkm::Id &ydim, 
                                 const vtkm::Id &zdim, 
                                 const PortalType &vec_data)
  {
    //get eight corner index
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;
    if (pos[0] < 0.0f)
      pos[0] = 0.0f;
    if (pos[0] > float(xdim - 1))
      pos[0] = float(xdim - 1);
    if (pos[1] < 0.0f)
      pos[1] = 0.0f;
    if (pos[1] > float(ydim - 1))
      pos[1] = float(ydim - 1);
    if (pos[2] < 0.0f)
      pos[2] = 0.0f;
    if (pos[2] > float(zdim - 1))
      pos[2] = float(zdim - 1);

    idx000[0] = int(floor(pos[0]));
    idx000[1] = int(floor(pos[1]));
    idx000[2] = int(floor(pos[2]));

    idx001 = idx000; idx001[0] = (idx001[0] + 1) <= xdim - 1 ? idx001[0] + 1 : xdim - 1;
    idx010 = idx000; idx010[1] = (idx010[1] + 1) <= ydim - 1 ? idx010[1] + 1 : ydim - 1;
    idx011 = idx010; idx011[0] = (idx011[0] + 1) <= xdim - 1 ? idx011[0] + 1 : xdim - 1;
    idx100 = idx000; idx100[2] = (idx100[2] + 1) <= zdim - 1 ? idx100[2] + 1 : zdim - 1;
    idx101 = idx100; idx101[0] = (idx101[0] + 1) <= xdim - 1 ? idx101[0] + 1 : xdim - 1;
    idx110 = idx100; idx110[1] = (idx110[1] + 1) <= ydim - 1 ? idx110[1] + 1 : ydim - 1;
    idx111 = idx110; idx111[0] = (idx111[0] + 1) <= xdim - 1 ? idx111[0] + 1 : xdim - 1;

#ifdef _Debug_2
  printf("v000 : %d, %d, %d\n", idx000[0], idx000[1], idx000[2]);
  printf("v001 : %d, %d, %d\n", idx001[0], idx001[1], idx001[2]);
  printf("v010 : %d, %d, %d\n", idx010[0], idx010[1], idx010[2]);
  printf("v011 : %d, %d, %d\n", idx011[0], idx011[1], idx011[2]);
  printf("v100 : %d, %d, %d\n", idx100[0], idx100[1], idx100[2]);
  printf("v101 : %d, %d, %d\n", idx101[0], idx101[1], idx101[2]);
  printf("v110 : %d, %d, %d\n", idx110[0], idx110[1], idx110[2]);
  printf("v111 : %d, %d, %d\n", idx111[0], idx111[1], idx111[2]);
#endif

    //get velocity
    vtkm::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
    v000 = GetVel<FieldType, PortalType>(idx000, xdim, ydim, zdim, vec_data);
    v001 = GetVel<FieldType, PortalType>(idx001, xdim, ydim, zdim, vec_data);
    v010 = GetVel<FieldType, PortalType>(idx010, xdim, ydim, zdim, vec_data);
    v011 = GetVel<FieldType, PortalType>(idx011, xdim, ydim, zdim, vec_data);
    v100 = GetVel<FieldType, PortalType>(idx100, xdim, ydim, zdim, vec_data);
    v101 = GetVel<FieldType, PortalType>(idx101, xdim, ydim, zdim, vec_data);
    v110 = GetVel<FieldType, PortalType>(idx110, xdim, ydim, zdim, vec_data);
    v111 = GetVel<FieldType, PortalType>(idx111, xdim, ydim, zdim, vec_data);

#ifdef _Debug_2
  printf("v000 vel: %d, %d, %d, %f, %f, %f\n", idx000[0], idx000[1], idx000[2], v000[0], v000[1], v000[2]);
  printf("v001 vel: %d, %d, %d, %f, %f, %f\n", idx001[0], idx001[1], idx001[2], v001[0], v001[1], v001[2]);
  printf("v010 vel: %d, %d, %d, %f, %f, %f\n", idx010[0], idx010[1], idx010[2], v010[0], v010[1], v010[2]);
  printf("v011 vel: %d, %d, %d, %f, %f, %f\n", idx011[0], idx011[1], idx011[2], v011[0], v011[1], v011[2]);
  printf("v100 vel: %d, %d, %d, %f, %f, %f\n", idx100[0], idx100[1], idx100[2], v100[0], v100[1], v100[2]);
  printf("v101 vel: %d, %d, %d, %f, %f, %f\n", idx101[0], idx101[1], idx101[2], v101[0], v101[1], v101[2]);
  printf("v110 vel: %d, %d, %d, %f, %f, %f\n", idx110[0], idx110[1], idx110[2], v110[0], v110[1], v110[2]);
  printf("v111 vel: %d, %d, %d, %f, %f, %f\n", idx111[0], idx111[1], idx111[2], v111[0], v111[1], v111[2]);
#endif

    //interpolation
    vtkm::Vec<FieldType, 3> v00, v01, v10, v11;
    float a = pos[0] - floor(pos[0]);
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

    vtkm::Vec<FieldType, 3> v0, v1;
    a = pos[1] - floor(pos[1]);
    v0[0] = (1.0f - a) * v00[0] + a * v01[0];
    v0[1] = (1.0f - a) * v00[1] + a * v01[1];
    v0[2] = (1.0f - a) * v00[2] + a * v01[2];

    v1[0] = (1.0f - a) * v10[0] + a * v11[0];
    v1[1] = (1.0f - a) * v10[1] + a * v11[1];
    v1[2] = (1.0f - a) * v10[2] + a * v11[2];

    vtkm::Vec<FieldType, 3> v;
    a = pos[2] - floor(pos[2]);
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
  typedef typename FieldHandle::ExecutionTypes<DeviceAdapter>::Portal FieldPortalType;
  typedef typename FieldHandle::ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;

  class MakeStreamLines : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputSeedId);
    typedef void ExecutionSignature(_1);
    typedef _1 InputDomain;

    FieldPortalConstType field_;
    FieldPortalConstType seeds_;
    FieldPortalType slLists_;

    const vtkm::Id xdim_, ydim_, zdim_, max_steps_;
    const FieldType t_;

    VTKM_CONT_EXPORT
    MakeStreamLines(const FieldType t, 
                    const vtkm::Id max_steps, 
                    const vtkm::Id dims[3], 
                    FieldPortalConstType field, 
                    FieldPortalConstType seeds, 
                    FieldPortalType slLists): 
                                  t_(t), 
                                  max_steps_(max_steps), 
                                  xdim_(dims[0]), 
                                  ydim_(dims[1]), 
                                  zdim_(dims[2]), 
                                  field_(field), 
                                  seeds_(seeds), 
                                  slLists_(slLists)
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id &seedId) const
    {
      vtkm::Vec<FieldType, 3> pre_pos = seeds_.Get(seedId);
      vtkm::Vec<FieldType, 3> pos = seeds_.Get(seedId);
      this->slLists_.Set(seedId * max_steps_ * 2, pos);

      // Forward tracing
      for (int i = 1; i < max_steps_; i++)
      {
        vtkm::Vec<FieldType, 3> vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        vtkm::Vec<FieldType, 3> a, b, c, d;
        a[0] = t_ * vel[0]; a[1] = t_ * vel[1]; a[2] = t_ * vel[2];
#ifdef _Debug
      printf("a pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("a vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        pos[0] += a[0] / 2.0f; pos[1] += a[1] / 2.0f; pos[2] += a[2] / 2.0f;
        vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        b[0] = t_ * vel[0]; b[1] = t_ * vel[1]; b[2] = t_ * vel[2];
#ifdef _Debug
      printf("b pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("b vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        pos[0] += b[0] / 2.0f; pos[1] += b[1] / 2.0f; pos[2] += b[2] / 2.0f;
        vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        c[0] = t_ * vel[0]; c[1] = t_ * vel[1]; c[2] = t_ * vel[2];
#ifdef _Debug
      printf("c pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("c vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        pos[0] += c[0] / 2.0f; pos[1] += c[1] / 2.0f; pos[2] += c[2] / 2.0f;
        vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        d[0] = t_ * vel[0]; d[1] = t_ * vel[1]; d[2] = t_ * vel[2];
#ifdef _Debug
      printf("d pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("d vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

          vtkm::Vec<FieldType, 3> nextPos;
          pos[0] = pos[0] + (a[0] + 2.0f * b[0] + 2.0f * c[0] + d[0]) / 6.0f;
          pos[1] = pos[1] + (a[1] + 2.0f * b[1] + 2.0f * c[1] + d[1]) / 6.0f;
          pos[2] = pos[2] + (a[2] + 2.0f * b[2] + 2.0f * c[2] + d[2]) / 6.0f;
#ifdef _Debug
      printf("next pos: %f, %f, %f\n", nextPos[0], nextPos[1], nextPos[2]);
#endif
        if (pos[0] < 0.0f || pos[0] > xdim_ || pos[1] < 0.0f || pos[1] > ydim_ || pos[2] < 0.0f || pos[2] > zdim_)
        {
          pos = pre_pos;
        }
        this->slLists_.Set(seedId * 2 * max_steps_ + i, pos);
        pre_pos = pos;
      }

      // Backward tracing
      pre_pos = seeds_.Get(seedId);
      pos = seeds_.Get(seedId);
      this->slLists_.Set((seedId * 2 + 1)*max_steps_, pos);
      for (int i = 1; i < max_steps_; i++)
      {
        vtkm::Vec<FieldType, 3> vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        vtkm::Vec<FieldType, 3> a, b, c, d;
        a[0] = t_ * (0.0f - vel[0]); a[1] = t_ * (0.0f - vel[1]); a[2] = t_ * (0.0f - vel[2]);
#ifdef _Debug_2
      printf("a pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("a vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        pos[0] += a[0] / 2.0f; pos[1] += a[1] / 2.0f; pos[2] += a[2] / 2.0f;
        vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        b[0] = t_ * (0.0f - vel[0]); b[1] = t_ * (0.0f - vel[1]); b[2] = t_ * (0.0f - vel[2]);
#ifdef _Debug_2
      printf("b pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("b vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        pos[0] += b[0] / 2.0f; pos[1] += b[1] / 2.0f; pos[2] += b[2] / 2.0f;
        vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        c[0] = t_ * (0.0f - vel[0]); c[1] = t_ * (0.0f - vel[1]); c[2] = t_ * (0.0f - vel[2]);
#ifdef _Debug_2
      printf("c pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("c vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        pos[0] += c[0] / 2.0f; pos[1] += c[1] / 2.0f; pos[2] += c[2] / 2.0f;
        vel = internal::AtPhys<FieldType, FieldPortalConstType>(pos, xdim_, ydim_, zdim_, field_);
        d[0] = t_ * (0.0f - vel[0]); d[1] = t_ * (0.0f - vel[1]); d[2] = t_ * (0.0f - vel[2]);
#ifdef _Debug_2
      printf("d pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
      printf("d vel: %f, %f, %f\n", vel[0], vel[1], vel[2]);
#endif

        vtkm::Vec<FieldType, 3> nextPos;
        pos[0] = pos[0] + (a[0] + 2.0f * b[0] + 2.0f * c[0] + d[0]) / 6.0f;
        pos[1] = pos[1] + (a[1] + 2.0f * b[1] + 2.0f * c[1] + d[1]) / 6.0f;
        pos[2] = pos[2] + (a[2] + 2.0f * b[2] + 2.0f * c[2] + d[2]) / 6.0f;
#ifdef _Debug_2
      printf("next pos: %f, %f, %f\n", nextPos[0], nextPos[1], nextPos[2]);
#endif
        if (pos[0] < 0.0f || pos[0] > xdim_ || pos[1] < 0.0f || pos[1] > ydim_ || pos[2] < 0.0f || pos[2] > zdim_)
        {
          pos = pre_pos;
        }
        this->slLists_.Set((seedId * 2 + 1)*max_steps_ + i, pos);
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

  template <typename FieldType>
  void Run(const FieldType t, 
           const vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > fieldArray,
           vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > slLists_)
  {
    // Generate random seeds for starting streamlines
    std::vector<vtkm::Vec<FieldType, 3> > seeds;

    for (vtkm::Id i = 0; i < g_num_seeds; i++)
    {
      vtkm::Vec<FieldType, 3> secretSeed;
      secretSeed[0] = rand() % g_dim[0];
      secretSeed[1] = rand() % g_dim[1];
      secretSeed[2] = rand() % g_dim[2];
      seeds.push_back(secretSeed);
printf("Seed %d = (%f, %f, %f)\n", i, secretSeed[0], secretSeed[1], secretSeed[2]);
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
                                    slLists_.PrepareForOutput(totalNumParticles, DeviceAdapter()));
    typedef typename vtkm::worklet::DispatcherMapField<MakeStreamLines> MakeStreamLinesDispatcher;
    MakeStreamLinesDispatcher makeStreamLinesDispatcher(makeStreamLines);
    makeStreamLinesDispatcher.Invoke(seedIdArray);
  }
};

}
}

#endif // vtk_m_worklet_StreamLineUniformGrid_h
