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
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/DataSet.h>
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
  {
  }

  VTKM_EXEC_CONT
  RK4Integrator(const FieldEvaluateType& field, FieldType _h, vtkm::cont::DataSet& dataset)
    : f(field)
    , h(_h)
  {
    bounds = dataset.GetCoordinateSystem(0).GetBounds();
  }

  template <typename PortalType>
  VTKM_EXEC ParticleStatus Step(const vtkm::Vec<FieldType, 3>& pos,
                                const PortalType& field,
                                vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
    vtkm::Vec<FieldType, 3> k1, k2, k3, k4;
    vtkm::Id numShortSteps = 0;
    bool shortSteps = false;
    FieldType step_h = h;
    FieldType step_h_2 = step_h / 2;
    if (f.Evaluate(pos, field, k1) && f.Evaluate(pos + step_h_2 * k1, field, k2) &&
        f.Evaluate(pos + step_h_2 * k2, field, k3) && f.Evaluate(pos + step_h * k3, field, k4))
    {
      /*
       * If the particle is inside bounds after taking the steps
       * return that the step was successful.
       */
      out = pos + step_h / 6.0f * (k1 + 2 * k2 + 2 * k3 + k4);
      return ParticleStatus::STATUS_OK;
    }
    else
    {
      shortSteps = true;
    }
    /*Take short steps to minimize advection error*/
    while (shortSteps)
    {
      /*reduce the step size to half*/
      step_h /= 2;
      step_h_2 = step_h / 2;
      if (f.Evaluate(pos, field, k1) && f.Evaluate(pos + step_h_2 * k1, field, k2) &&
          f.Evaluate(pos + step_h_2 * k2, field, k3) && f.Evaluate(pos + step_h * k3, field, k4))
      {
        out = pos + step_h / 6.0f * (k1 + 2 * k2 + 2 * k3 + k4);
        numShortSteps++;
      }
      /*
        * At this time the particle is still inside the bounds
        * To Push it at/out of the boundary take an Euler step
        */
      /*Check for the function like VisIt*/
      if (numShortSteps == 2)
      {
        step_h /= 2;
        step_h_2 = step_h / 2;
        /*Calculate the velocity of the particle at current position*/
        f.Evaluate(out, field, k1);
        f.Evaluate(out + step_h_2 * k1, field, k2);
        f.Evaluate(out + step_h_2 * k2, field, k3);
        f.Evaluate(out + step_h * k3, field, k4);
        vtkm::Vec<FieldType, 3> vel = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
        /*Get the direction of the particle*/
        FieldType magnitude = vtkm::Magnitude(vel);
        vtkm::Vec<FieldType, 3> dir = vel / magnitude;
        /*Determine the bounds for the particle*/
        FieldType xbound = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
        FieldType ybound = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
        FieldType zbound = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
        /*
         * Determine the minimum travel time for the
         * particle to reach the boundary
         */
        FieldType hx = std::abs(xbound - out[0]) / std::abs(vel[0]);
        FieldType hy = std::abs(ybound - out[1]) / std::abs(vel[1]);
        FieldType hz = std::abs(zbound - out[2]) / std::abs(vel[2]);
        FieldType hesc = std::min(hx, std::min(hy, hz));
        hesc += hesc / 100.0f;
        out += hesc * vel;
        shortSteps = false;
        break;
      }
    }
    return ParticleStatus::EXITED_SPATIAL_BOUNDARY;
  }

  FieldEvaluateType f;
  FieldType h;
  vtkm::Bounds bounds;
}; //RK4Integrator

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
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_Integrators_h
