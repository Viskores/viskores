//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtk_m_worklet_particleadvection_Integrator_Status_h
#define vtk_m_worklet_particleadvection_Integrator_Status_h

#include <iomanip>
#include <limits>

#include <vtkm/Bitset.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/worklet/particleadvection/GridEvaluatorStatus.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

class IntegratorStatus : public vtkm::Bitset<vtkm::UInt8>
{
public:
  VTKM_EXEC_CONT IntegratorStatus() {}

  VTKM_EXEC_CONT IntegratorStatus(const bool& ok,
                                  const bool& spatial,
                                  const bool& temporal,
                                  const bool& inGhost)
  {
    this->set(this->SUCCESS_BIT, ok);
    this->set(this->SPATIAL_BOUNDS_BIT, spatial);
    this->set(this->TEMPORAL_BOUNDS_BIT, temporal);
    this->set(this->IN_GHOST_CELL_BIT, inGhost);
  }

  VTKM_EXEC_CONT IntegratorStatus(const GridEvaluatorStatus& es)
  {
    this->set(this->SUCCESS_BIT, es.CheckOk());
    this->set(this->SPATIAL_BOUNDS_BIT, es.CheckSpatialBounds());
    this->set(this->TEMPORAL_BOUNDS_BIT, es.CheckTemporalBounds());
    this->set(this->IN_GHOST_CELL_BIT, es.CheckInGhostCell());
  }

  VTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(this->SPATIAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(this->TEMPORAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetInGhostCell() { this->set(this->IN_GHOST_CELL_BIT); }
  VTKM_EXEC_CONT bool CheckInGhostCell() const { return this->test(this->IN_GHOST_CELL_BIT); }

private:
  static constexpr vtkm::Id SUCCESS_BIT = 0;
  static constexpr vtkm::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr vtkm::Id TEMPORAL_BOUNDS_BIT = 2;
  static constexpr vtkm::Id IN_GHOST_CELL_BIT = 3;
};

inline VTKM_CONT std::ostream& operator<<(std::ostream& s, const IntegratorStatus& status)
{
  s << "[ok= " << status.CheckOk() << " sp= " << status.CheckSpatialBounds()
    << " tm= " << status.CheckTemporalBounds() << " gc= " << status.CheckInGhostCell() << "]";
  return s;
}
}
}
}


#endif // vtk_m_worklet_particleadvection_IntegratorStatus_h
