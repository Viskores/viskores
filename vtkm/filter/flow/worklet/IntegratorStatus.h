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

#ifndef vtk_m_filter_flow_worklet_IntegratorStatus_h
#define vtk_m_filter_flow_worklet_IntegratorStatus_h

#include <iomanip>
#include <limits>

#include <vtkm/Bitset.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/filter/flow/worklet/GridEvaluatorStatus.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

class IntegratorStatus : public vtkm::Bitset<vtkm::UInt8>
{
public:
  VTKM_EXEC_CONT IntegratorStatus() {}

  VTKM_EXEC_CONT IntegratorStatus(const bool& ok,
                                  const bool& spatial,
                                  const bool& temporal,
                                  const bool& inGhost,
                                  const bool& isZero)
  {
    this->set(this->SUCCESS_BIT, ok);
    this->set(this->SPATIAL_BOUNDS_BIT, spatial);
    this->set(this->TEMPORAL_BOUNDS_BIT, temporal);
    this->set(this->IN_GHOST_CELL_BIT, inGhost);
    this->set(this->ZERO_VELOCITY_BIT, isZero);
  }

  VTKM_EXEC_CONT IntegratorStatus(const GridEvaluatorStatus& es, bool isZero)
    : IntegratorStatus(es.CheckOk(),
                       es.CheckSpatialBounds(),
                       es.CheckTemporalBounds(),
                       es.CheckInGhostCell(),
                       isZero)
  {
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
  VTKM_EXEC_CONT void SetZeroVelocity() { this->set(this->ZERO_VELOCITY_BIT); }
  VTKM_EXEC_CONT bool CheckZeroVelocity() const { return this->test(this->ZERO_VELOCITY_BIT); }

private:
  static constexpr vtkm::Id SUCCESS_BIT = 0;
  static constexpr vtkm::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr vtkm::Id TEMPORAL_BOUNDS_BIT = 2;
  static constexpr vtkm::Id IN_GHOST_CELL_BIT = 3;
  static constexpr vtkm::Id ZERO_VELOCITY_BIT = 4;
};

inline VTKM_CONT std::ostream& operator<<(std::ostream& s, const IntegratorStatus& status)
{
  s << "[ok= " << status.CheckOk() << " sp= " << status.CheckSpatialBounds()
    << " tm= " << status.CheckTemporalBounds() << " gc= " << status.CheckInGhostCell()
    << "zero= " << status.CheckZeroVelocity() << " ]";
  return s;
}

}
}
} //vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_IntegratorStatus_h
