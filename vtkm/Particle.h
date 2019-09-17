//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_Particle_h
#define vtk_m_Particle_h



namespace vtkm
{

enum ParticleStatus : vtkm::Id
{
  UNDEFINED = 0,
  SUCCESS = 1,
  TERMINATED = 1 << 1,
  EXIT_SPATIAL_BOUNDARY = 1 << 2,
  EXIT_TEMPORAL_BOUNDARY = 1 << 3,
  FAIL = 1 << 4,
  TOOK_ANY_STEPS = 1 << 5
};

class Particle
{
public:
  Particle() {}
  Particle(const vtkm::Vec3f& p, vtkm::Id id, vtkm::Id numSteps)
    : Pos(p)
    , ID(id)
    , NumSteps(numSteps)
    , Status(vtkm::ParticleStatus::UNDEFINED)
  {
  }

  vtkm::Vec3f Pos;
  vtkm::Id ID;
  vtkm::Id NumSteps;
  vtkm::Id Status;
  vtkm::FloatDefault Time;
};
}

#endif // vtk_m_Particle_h
