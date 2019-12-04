//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetExtrude_hxx
#define vtk_m_cont_CellSetExtrude_hxx

namespace
{
struct ComputeReverseMapping : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn cellIndex, WholeArrayOut cellIds);

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC void operator()(vtkm::Id cellId, PortalType&& pointIdValue) const
  {
    //3 as we are building the connectivity for triangles
    const vtkm::Id offset = 3 * cellId;
    pointIdValue.Set(offset, static_cast<vtkm::Int32>(cellId));
    pointIdValue.Set(offset + 1, static_cast<vtkm::Int32>(cellId));
    pointIdValue.Set(offset + 2, static_cast<vtkm::Int32>(cellId));
  }
};

struct ComputePrevNode : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn nextNode, WholeArrayOut prevNodeArray);
  typedef void ExecutionSignature(InputIndex, _1, _2);

  template <typename PortalType>
  VTKM_EXEC void operator()(vtkm::Id idx, vtkm::Int32 next, PortalType& prevs) const
  {
    prevs.Set(static_cast<vtkm::Id>(next), static_cast<vtkm::Int32>(idx));
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{
template <typename Device>
VTKM_CONT void CellSetExtrude::BuildReverseConnectivity(Device)
{
  vtkm::cont::Invoker invoke(Device{});

  // create a mapping of where each key is the point id and the value
  // is the cell id. We
  const vtkm::Id numberOfPointsPerCell = 3;
  const vtkm::Id rconnSize = this->NumberOfCellsPerPlane * numberOfPointsPerCell;

  vtkm::cont::ArrayHandle<vtkm::Int32> pointIdKey;
  vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(this->Connectivity, pointIdKey);

  this->RConnectivity.Allocate(rconnSize);
  invoke(ComputeReverseMapping{},
         vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, this->NumberOfCellsPerPlane),
         this->RConnectivity);

  vtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(pointIdKey, this->RConnectivity);

  // now we can compute the counts and offsets
  vtkm::cont::ArrayHandle<vtkm::Int32> reducedKeys;
  vtkm::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(
    pointIdKey,
    vtkm::cont::make_ArrayHandleConstant(vtkm::Int32(1), static_cast<vtkm::Int32>(rconnSize)),
    reducedKeys,
    this->RCounts,
    vtkm::Add{});

  vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(this->RCounts, this->ROffsets);

  // compute PrevNode from NextNode
  this->PrevNode.Allocate(this->NextNode.GetNumberOfValues());
  invoke(ComputePrevNode{}, this->NextNode, this->PrevNode);

  this->ReverseConnectivityBuilt = true;
}
template <typename Device>
CellSetExtrude::ConnectivityP2C<Device> CellSetExtrude::PrepareForInput(
  Device,
  vtkm::TopologyElementTagCell,
  vtkm::TopologyElementTagPoint) const
{
  return ConnectivityP2C<Device>(this->Connectivity.PrepareForInput(Device{}),
                                 this->NextNode.PrepareForInput(Device{}),
                                 this->NumberOfCellsPerPlane,
                                 this->NumberOfPointsPerPlane,
                                 this->NumberOfPlanes,
                                 this->IsPeriodic);
}


template <typename Device>
VTKM_CONT CellSetExtrude::ConnectivityC2P<Device> CellSetExtrude::PrepareForInput(
  Device,
  vtkm::TopologyElementTagPoint,
  vtkm::TopologyElementTagCell) const
{
  if (!this->ReverseConnectivityBuilt)
  {
    const_cast<CellSetExtrude*>(this)->BuildReverseConnectivity(Device{});
  }
  return ConnectivityC2P<Device>(this->RConnectivity.PrepareForInput(Device{}),
                                 this->ROffsets.PrepareForInput(Device{}),
                                 this->RCounts.PrepareForInput(Device{}),
                                 this->PrevNode.PrepareForInput(Device{}),
                                 this->NumberOfCellsPerPlane,
                                 this->NumberOfPointsPerPlane,
                                 this->NumberOfPlanes);
}
}
} // vtkm::cont
#endif
