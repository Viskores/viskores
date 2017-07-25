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

#ifndef vtk_m_worklet_particleadvection_Integrators_h
#define vtk_m_worklet_particleadvection_Integrators_h

#include <vtkm/Types.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename FieldEvaluateType, typename FieldType>
class RK4Integrator
{
public:
  VTKM_EXEC_CONT
  RK4Integrator()
    : h(0)
    , h_2(0)
  {
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& field, FieldType _h)
    : f(field)
    , h(_h)
    , h_2(_h / 2.f)
  {
  }

  template <typename PortalType>
  VTKM_EXEC ParticleStatus Step(const vtkm::Vec<FieldType, 3>& pos,
                                const PortalType& field,
                                vtkm::Vec<FieldType, 3>& out) const
  {
    vtkm::Vec<FieldType, 3> k1, k2, k3, k4;

    if (f.Evaluate(pos, field, k1) && f.Evaluate(pos + h_2 * k1, field, k2) &&
        f.Evaluate(pos + h_2 * k2, field, k3) && f.Evaluate(pos + h * k3, field, k4))
    {
      out = pos + h / 6.0f * (k1 + 2 * k2 + 2 * k3 + k4);
      return ParticleStatus::STATUS_OK;
    }
    return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

  FieldEvaluateType f;
  FieldType h, h_2;
};

template <typename FieldEvaluateType, typename FieldType>
class EulerIntegrator
{
public:
  VTKM_EXEC_CONT
  EulerIntegrator(const FieldEvaluateType& field, FieldType _h)
    : f(field)
    , h(_h)
  {
  }

  template <typename PortalType>
  VTKM_EXEC bool Step(const vtkm::Vec<FieldType, 3>& pos,
                      const PortalType& field,
                      vtkm::Vec<FieldType, 3>& out) const
  {
    vtkm::Vec<FieldType, 3> vCur;
    if (f.Evaluate(pos, field, vCur))
    {
      out = pos + h * vCur;
      return true;
    }
    return false;
  }

  FieldEvaluateType f;
  FieldType h;
};
}
}
}

#endif // vtk_m_worklet_particleadvection_Integrators_h
