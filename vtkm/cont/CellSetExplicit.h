//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/exec/ConnectivityExplicit.h>

namespace vtkm {
namespace cont {

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename NumIndicesStorageTag    = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
class CellSetExplicit : public CellSet
{
  typedef vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> ShapeArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, NumIndicesStorageTag> NumIndicesArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> ConnectivityArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id> MapCellToConnectivityIndexArrayType;

public:
  typedef vtkm::Id SchedulingRangeType;

  VTKM_CONT_EXPORT
  CellSetExplicit(const std::string &name = std::string(),
                  vtkm::IdComponent dimensionality = 3)
    : CellSet(name, dimensionality),
      ConnectivityLength(0),
      NumberOfCells(0)
  {
  }

  VTKM_CONT_EXPORT
  CellSetExplicit(int dimensionality)
    : CellSet(std::string(), dimensionality),
      ConnectivityLength(0),
      NumberOfCells(0)
  {
  }

  virtual vtkm::Id GetNumberOfCells() const
  {
    return this->Shapes.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfNodesInCell(vtkm::Id cellIndex) const
  {
    return this->NumIndices.GetPortalConstControl().Get(cellIndex);
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetCellShapeType(vtkm::Id cellIndex) const
  {
    return this->Shapes.GetPortalConstControl().Get(cellIndex);
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void GetIndices(vtkm::Id index,
                  vtkm::Vec<vtkm::Id,ItemTupleLength> &ids) const
  {
    vtkm::Id numIndices = this->GetNumberOfIndices(index);
    vtkm::Id start =
        this->MapCellToConnectivityIndex.GetPortalConstControl().Get(index);
    for (vtkm::IdComponent i=0; i<numIndices && i<ItemTupleLength; i++)
      ids[i] = this->Connectivity.GetPortalConstControl().Get(start+i);
  }

  /// First method to add cells -- one at a time.
  VTKM_CONT_EXPORT
  void PrepareToAddCells(vtkm::Id numShapes, vtkm::Id connectivityMaxLen)
  {
    this->Shapes.Allocate(numShapes);
    this->NumIndices.Allocate(numShapes);
    this->Connectivity.Allocate(connectivityMaxLen);
    this->MapCellToConnectivityIndex.Allocate(numShapes);
    this->NumberOfCells = 0;
    this->ConnectivityLength = 0;
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void AddCell(vtkm::CellType cellType,
               int numVertices,
               const vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    this->Shapes.GetPortalControl().Set(this->NumberOfCells, cellType);
    this->NumIndices.GetPortalControl().Set(this->NumberOfCells, numVertices);
    for (vtkm::IdComponent i=0; i < numVertices; ++i)
    {
      this->Connectivity.GetPortalControl().Set(
            this->ConnectivityLength+i,ids[i]);
    }
    this->MapCellToConnectivityIndex.GetPortalControl().Set(
          this->NumberOfCells, this->ConnectivityLength);
    this->NumberOfCells++;
    this->ConnectivityLength += numVertices;
  }

  VTKM_CONT_EXPORT
  void CompleteAddingCells()
  {
    Connectivity.Shrink(ConnectivityLength);
  }

  /// Second method to add cells -- all at once.
  /// Copies the data from the vectors, so they can be released.
  VTKM_CONT_EXPORT
  void FillViaCopy(const std::vector<vtkm::Id> &cellTypes,
                   const std::vector<vtkm::Id> &numIndices,
                   const std::vector<vtkm::Id> &connectivity)
  {

    this->Shapes.Allocate( static_cast<vtkm::Id>(cellTypes.size()) );
    std::copy(cellTypes.begin(), cellTypes.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(this->Shapes.GetPortalControl()));

    this->NumIndices.Allocate( static_cast<vtkm::Id>(numIndices.size()) );
    std::copy(numIndices.begin(), numIndices.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(this->NumIndices.GetPortalControl()));

    this->Connectivity.Allocate( static_cast<vtkm::Id>(connectivity.size()) );
    std::copy(connectivity.begin(), connectivity.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(this->Connectivity.GetPortalControl()));

    this->NumberOfCells = this->Shapes.GetNumberOfValues();
    this->ConnectivityLength = this->Connectivity.GetNumberOfValues();

    // allocate and build reverse index
    this->MapCellToConnectivityIndex.Allocate(this->NumberOfCells);
    vtkm::Id counter = 0;
    for (vtkm::Id i=0; i<this->NumberOfCells; ++i)
    {
      this->MapCellToConnectivityIndex.GetPortalControl().Set(i, counter);
      counter += this->NumIndices.GetPortalConstControl().Get(i);
    }
  }

  /// Second method to add cells -- all at once.
  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  void Fill(const vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> &cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::Id, NumIndicesStorageTag> &numIndices,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &connectivity)
  {
    this->Shapes = cellTypes;
    this->NumIndices = numIndices;
    this->Connectivity = connectivity;

    this->NumberOfCells = this->Shapes.GetNumberOfValues();
    this->ConnectivityLength = this->Connectivity.GetNumberOfValues();

    // allocate and build reverse index
    this->MapCellToConnectivityIndex.Allocate(this->NumberOfCells);
    vtkm::Id counter = 0;
    for (vtkm::Id i=0; i<this->NumberOfCells; ++i)
    {
      this->MapCellToConnectivityIndex.GetPortalControl().Set(i, counter);
      counter += this->NumIndices.GetPortalConstControl().Get(i);
    }
  }

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef typename ShapeArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst ShapePortalType;
    typedef typename NumIndicesArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst IndicePortalType;
    typedef typename ConnectivityArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst ConnectivityPortalType;
    typedef typename MapCellToConnectivityIndexArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst MapConnectivityPortalType;

    typedef vtkm::exec::ConnectivityExplicit<ShapePortalType,
                                             IndicePortalType,
                                             ConnectivityPortalType,
                                             MapConnectivityPortalType
                                             > ExecObjectType;
  };

  template<typename Device>
  typename ExecutionTypes<Device,vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell>::ExecObjectType
  PrepareForInput(Device,
                  vtkm::TopologyElementTagPoint,
                  vtkm::TopologyElementTagCell) const
  {
    typedef typename ExecutionTypes<Device,vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell>::ExecObjectType ExecObjType;
    return ExecObjType(this->Shapes.PrepareForInput(Device()),
                       this->NumIndices.PrepareForInput(Device()),
                       this->Connectivity.PrepareForInput(Device()),
                       this->MapCellToConnectivityIndex.PrepareForInput(Device()));
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out << "   ExplicitCellSet: " << this->Name
          << " dim= " << this->Dimensionality << std::endl;
      out <<"     Shapes: ";
      vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << std::endl;
      out << "     NumIndices: ";
      vtkm::cont::printSummary_ArrayHandle(this->NumIndices, out);
      out << std::endl;
      out << "     Connectivity: ";
      vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
      out << std::endl;
      out << "     MapCellToConnectivityIndex: ";
      vtkm::cont::printSummary_ArrayHandle(
            this->MapCellToConnectivityIndex, out);
  }

  VTKM_CONT_EXPORT
  vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> &
  GetShapesArray() const { return this->Shapes; }

  VTKM_CONT_EXPORT
  vtkm::cont::ArrayHandle<vtkm::Id, NumIndicesStorageTag> &
  GetNumIndicesArray() const { return this->NumIndices; }

  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &
  GetConnectivityArray() const { return this->Connectivity; }

  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Id> &
  GetCellToConnectivityIndexArray() const {
    return this->MapCellToConnectivityIndex;
  }

public:
  vtkm::Id ConnectivityLength;
  vtkm::Id NumberOfCells;
  ShapeArrayType Shapes;
  NumIndicesArrayType NumIndices;
  ConnectivityArrayType Connectivity;
  MapCellToConnectivityIndexArrayType MapCellToConnectivityIndex;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
