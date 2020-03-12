//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_GridEvaluatorStatus_h
#define vtk_m_worklet_particleadvection_GridEvaluatorStatus_h

#include <vtkm/Bitset.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/worklet/particleadvection/CellInterpolationHelper.h>
#include <vtkm/worklet/particleadvection/Integrators.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

class GridEvaluatorStatus : public vtkm::Bitset<vtkm::UInt8>
{
public:
  VTKM_EXEC_CONT GridEvaluatorStatus(){};
  VTKM_EXEC_CONT GridEvaluatorStatus(bool ok, bool spatial, bool temporal)
  {
    this->set(this->SUCCESS_BIT, ok);
    this->set(this->SPATIAL_BOUNDS_BIT, spatial);
    this->set(this->TEMPORAL_BOUNDS_BIT, temporal);
  };
  VTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  VTKM_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  VTKM_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(this->SPATIAL_BOUNDS_BIT); }

  VTKM_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  VTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(this->TEMPORAL_BOUNDS_BIT); }

private:
  static constexpr vtkm::Id SUCCESS_BIT = 0;
  static constexpr vtkm::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr vtkm::Id TEMPORAL_BOUNDS_BIT = 2;
};
}
}
}

#endif // vtk_m_worklet_particleadvection_GridEvaluatorStatus_h
