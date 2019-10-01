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

#include <vtkm/Bitset.h>

namespace vtkm
{

//Bit field describing the status:
class ParticleStatus : public vtkm::Bitset<vtkm::UInt8>
{
public:
  VTKM_EXEC_CONT ParticleStatus()
  {
    this->SetOk();
    this->ClearTerminate();
  }

  VTKM_EXEC_CONT void SetOk() { this->set(SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckOk() const { return this->test(SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetFail() { this->reset(SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckFail() const { return !this->test(SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetTerminate() { this->set(TERMINATE_BIT); }
  VTKM_EXEC_CONT void ClearTerminate() { this->reset(TERMINATE_BIT); }
  VTKM_EXEC_CONT bool CheckTerminate() const { return this->test(TERMINATE_BIT); }

  VTKM_EXEC_CONT void SetSpatialBounds() { this->set(SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT void ClearSpatialBounds() { this->reset(SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(SPATIAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTemporalBounds() { this->set(TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT void ClearTemporalBounds() { this->reset(TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(TEMPORAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTookAnySteps() { this->set(TOOK_ANY_STEPS_BIT); }
  VTKM_EXEC_CONT void ClearTookAnySteps() { this->reset(TOOK_ANY_STEPS_BIT); }
  VTKM_EXEC_CONT bool CheckTookAnySteps() const { return this->test(TOOK_ANY_STEPS_BIT); }

private:
  static constexpr vtkm::IdComponent SUCCESS_BIT = 0;
  static constexpr vtkm::IdComponent TERMINATE_BIT = 1;
  static constexpr vtkm::IdComponent SPATIAL_BOUNDS_BIT = 2;
  static constexpr vtkm::IdComponent TEMPORAL_BOUNDS_BIT = 3;
  static constexpr vtkm::IdComponent TOOK_ANY_STEPS_BIT = 4;
};

inline VTKM_EXEC_CONT std::ostream& operator<<(std::ostream& s, const vtkm::ParticleStatus& status)
{
  s << "[" << status.CheckOk() << " " << status.CheckTerminate() << " "
    << status.CheckSpatialBounds() << " " << status.CheckTemporalBounds() << "]";
  return s;
}

class Particle
{
public:
  VTKM_EXEC_CONT
  Particle()
    : Pos()
    , ID(-1)
    , NumSteps(0)
    , Status()
    , Time(0)
  {
  }

  VTKM_EXEC_CONT
  Particle(const vtkm::Particle& p)
    : Pos(p.Pos)
    , ID(p.ID)
    , NumSteps(p.NumSteps)
    , Status(p.Status)
    , Time(p.Time)
  {
  }

  VTKM_EXEC_CONT
  Particle(const vtkm::Vec3f& p,
           const vtkm::Id& id,
           const vtkm::Id& numSteps = 0,
           const vtkm::ParticleStatus& status = vtkm::ParticleStatus(),
           const vtkm::FloatDefault& time = 0)
    : Pos(p)
    , ID(id)
    , NumSteps(numSteps)
    , Status(status)
    , Time(time)
  {
  }

  vtkm::Vec3f Pos;
  vtkm::Id ID;
  vtkm::Id NumSteps;
  vtkm::ParticleStatus Status;
  vtkm::FloatDefault Time;
};
}

#endif // vtk_m_Particle_h
