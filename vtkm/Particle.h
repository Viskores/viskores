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

  VTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetTerminate() { this->set(this->TERMINATE_BIT); }
  VTKM_EXEC_CONT void ClearTerminate() { this->reset(this->TERMINATE_BIT); }
  VTKM_EXEC_CONT bool CheckTerminate() const { return this->test(this->TERMINATE_BIT); }

  VTKM_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT void ClearSpatialBounds() { this->reset(this->SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(this->SPATIAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT void ClearTemporalBounds() { this->reset(this->TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(this->TEMPORAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTookAnySteps() { this->set(this->TOOK_ANY_STEPS_BIT); }
  VTKM_EXEC_CONT void ClearTookAnySteps() { this->reset(this->TOOK_ANY_STEPS_BIT); }
  VTKM_EXEC_CONT bool CheckTookAnySteps() const { return this->test(this->TOOK_ANY_STEPS_BIT); }

private:
  static constexpr vtkm::Id SUCCESS_BIT = 0;
  static constexpr vtkm::Id TERMINATE_BIT = 1;
  static constexpr vtkm::Id SPATIAL_BOUNDS_BIT = 2;
  static constexpr vtkm::Id TEMPORAL_BOUNDS_BIT = 3;
  static constexpr vtkm::Id TOOK_ANY_STEPS_BIT = 4;
};

inline VTKM_CONT std::ostream& operator<<(std::ostream& s, const vtkm::ParticleStatus& status)
{
  s << "[" << status.CheckOk() << " " << status.CheckTerminate() << " "
    << status.CheckSpatialBounds() << " " << status.CheckTemporalBounds() << "]";
  return s;
}

class Particle
{
public:
  VTKM_EXEC_CONT
  Particle() {}

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

  VTKM_EXEC_CONT
  Particle(const vtkm::Particle& p)
    : Pos(p.Pos)
    , ID(p.ID)
    , NumSteps(p.NumSteps)
    , Status(p.Status)
    , Time(p.Time)
  {
  }

  vtkm::Particle& operator=(const vtkm::Particle& p) = default;

  vtkm::Vec3f Pos;
  vtkm::Id ID = -1;
  vtkm::Id NumSteps = 0;
  vtkm::ParticleStatus Status;
  vtkm::FloatDefault Time = 0;
};
}

#endif // vtk_m_Particle_h
