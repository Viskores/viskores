//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetExtrude_h
#define vtk_m_cont_CellSetExtrude_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleExtrudeCoords.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/exec/ConnectivityExtrude.h>
#include <vtkm/exec/arg/ThreadIndicesExtrude.h>
#include <vtkm/worklet/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellSetExtrude : public CellSet
{
public:
  VTKM_CONT CellSetExtrude(const std::string& name = "extrude");

  VTKM_CONT CellSetExtrude(const vtkm::cont::ArrayHandle<vtkm::Int32>& conn,
                           vtkm::Int32 numberOfPointsPerPlane,
                           vtkm::Int32 numberOfPlanes,
                           const vtkm::cont::ArrayHandle<vtkm::Int32>& nextNode,
                           bool periodic,
                           const std::string& name = "extrude");

  vtkm::Int32 GetNumberOfPlanes() const;

  vtkm::Id GetNumberOfCells() const override;

  vtkm::Id GetNumberOfPoints() const override;

  vtkm::Id GetNumberOfFaces() const override;

  vtkm::Id GetNumberOfEdges() const override;

  VTKM_CONT vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagCell) const;

  VTKM_CONT vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagPoint) const;

  vtkm::UInt8 GetCellShape(vtkm::Id id) const override;
  vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id id) const override;
  void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const override;

  std::shared_ptr<CellSet> NewInstance() const override;
  void DeepCopy(const CellSet* src) override;

  void PrintSummary(std::ostream& out) const override;
  void ReleaseResourcesExecution() override;


  template <typename DeviceAdapter>
  using ConnectivityP2C = vtkm::exec::ConnectivityExtrude<DeviceAdapter>;
  template <typename DeviceAdapter>
  using ConnectivityC2P = vtkm::exec::ReverseConnectivityExtrude<DeviceAdapter>;

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes;

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>
  {
    using ExecObjectType = ConnectivityP2C<DeviceAdapter>;
  };

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>
  {
    using ExecObjectType = ConnectivityC2P<DeviceAdapter>;
  };

  template <typename Device>
  ConnectivityP2C<Device> PrepareForInput(Device,
                                          vtkm::TopologyElementTagPoint,
                                          vtkm::TopologyElementTagCell) const;

  template <typename Device>
  ConnectivityC2P<Device> PrepareForInput(Device,
                                          vtkm::TopologyElementTagCell,
                                          vtkm::TopologyElementTagPoint) const;

private:
  template <typename Device>
  void BuildReverseConnectivity(Device);

  bool IsPeriodic;

  vtkm::Int32 NumberOfPointsPerPlane;
  vtkm::Int32 NumberOfCellsPerPlane;
  vtkm::Int32 NumberOfPlanes;
  vtkm::cont::ArrayHandle<vtkm::Int32> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Int32> NextNode;

  bool ReverseConnectivityBuilt;
  vtkm::cont::ArrayHandle<vtkm::Int32> RConnectivity;
  vtkm::cont::ArrayHandle<vtkm::Int32> ROffsets;
  vtkm::cont::ArrayHandle<vtkm::Int32> RCounts;
  vtkm::cont::ArrayHandle<vtkm::Int32> PrevNode;
};

template <typename T>
CellSetExtrude make_CellSetExtrude(const vtkm::cont::ArrayHandle<vtkm::Int32>& conn,
                                   const vtkm::cont::ArrayHandleExtrudeCoords<T>& coords,
                                   const vtkm::cont::ArrayHandle<vtkm::Int32>& nextNode,
                                   bool periodic = true,
                                   const std::string name = "extrude")
{
  return CellSetExtrude{
    conn, coords.GetNumberOfPointsPerPlane(), coords.GetNumberOfPlanes(), nextNode, periodic, name
  };
}

template <typename T>
CellSetExtrude make_CellSetExtrude(const std::vector<vtkm::Int32>& conn,
                                   const vtkm::cont::ArrayHandleExtrudeCoords<T>& coords,
                                   const std::vector<vtkm::Int32>& nextNode,
                                   bool periodic = true,
                                   const std::string name = "extrude")
{
  return CellSetExtrude{ vtkm::cont::make_ArrayHandle(conn),
                         static_cast<vtkm::Int32>(coords.GetNumberOfPointsPerPlane()),
                         coords.GetNumberOfPlanes(),
                         vtkm::cont::make_ArrayHandle(nextNode),
                         periodic,
                         name };
}
}
} // vtkm::cont

#include <vtkm/cont/CellSetExtrude.hxx>
#endif
