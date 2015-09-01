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

#include <vtkm/CellShape.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <vtkm/exec/ConnectivityExplicit.h>

#include <map>
#include <utility>

namespace vtkm {
namespace cont {

namespace detail {

template<typename CellSetType, typename FromTopology, typename ToTopology>
struct CellSetExplicitConnectivityChooser
{
  typedef vtkm::cont::internal::ConnectivityExplicitInternals<>
      ConnectivityType;
};

} // namespace detail

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename NumIndicesStorageTag    = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
class CellSetExplicit : public CellSet
{
  template<typename FromTopology, typename ToTopology>
  struct ConnectivityChooser
  {
    typedef typename detail::CellSetExplicitConnectivityChooser<
        CellSetExplicit<
          ShapeStorageTag,NumIndicesStorageTag,ConnectivityStorageTag>,
        FromTopology,
        ToTopology>::ConnectivityType ConnectivityType;
  };

public:
  typedef vtkm::Id SchedulingRangeType;

  VTKM_CONT_EXPORT
  CellSetExplicit(vtkm::Id numpoints = 0,
                  const std::string &name = std::string(),
                  vtkm::IdComponent dimensionality = 3)
    : CellSet(name, dimensionality),
      NumberOfPoints(numpoints),
      ConnectivityLength(-1),
      NumberOfCells(-1)
  {
  }

  VTKM_CONT_EXPORT
  CellSetExplicit(vtkm::Id numpoints, int dimensionality)
    : CellSet(std::string(), dimensionality),
      NumberOfPoints(numpoints),
      ConnectivityLength(-1),
      NumberOfCells(-1)
  {
  }

  virtual vtkm::Id GetNumberOfCells() const
  {
    return this->PointToCell.GetNumberOfElements();
  }

  virtual vtkm::Id GetNumberOfPoints() const
  {
    return this->NumberOfPoints;
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfPointsInCell(vtkm::Id cellIndex) const
  {
    return this->PointToCell.NumIndices.GetPortalConstControl().Get(cellIndex);
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetCellShape(vtkm::Id cellIndex) const
  {
    return this->PointToCell.Shapes.GetPortalConstControl().Get(cellIndex);
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void GetIndices(vtkm::Id index,
                  vtkm::Vec<vtkm::Id,ItemTupleLength> &ids) const
  {
    this->PointToCell.BuildIndexOffsets(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    vtkm::Id numIndices = this->GetNumberOfIndices(index);
    vtkm::Id start =
        this->PointToCell.IndexOffset.GetPortalConstControl().Get(index);
    for (vtkm::IdComponent i=0; i<numIndices && i<ItemTupleLength; i++)
      ids[i] = this->Connectivity.GetPortalConstControl().Get(start+i);
  }

  /// First method to add cells -- one at a time.
  VTKM_CONT_EXPORT
  void PrepareToAddCells(vtkm::Id numShapes, vtkm::Id connectivityMaxLen)
  {
    this->PointToCell.Shapes.Allocate(numShapes);
    this->PointToCell.NumIndices.Allocate(numShapes);
    this->PointToCell.Connectivity.Allocate(connectivityMaxLen);
    this->PointToCell.IndexOffsets.Allocate(numShapes);
    this->NumberOfCells = 0;
    this->ConnectivityLength = 0;
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void AddCell(vtkm::IdComponent cellType,
               int numVertices,
               const vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    this->PointToCell.Shapes.GetPortalControl().Set(this->NumberOfCells, cellType);
    this->PointToCell.NumIndices.GetPortalControl().Set(this->NumberOfCells, numVertices);
    for (vtkm::IdComponent i=0; i < numVertices; ++i)
    {
      this->PointToCell.Connectivity.GetPortalControl().Set(
            this->ConnectivityLength+i,ids[i]);
    }
    this->PointToCell.IndexOffsets.GetPortalControl().Set(
          this->NumberOfCells, this->ConnectivityLength);
    this->NumberOfCells++;
    this->ConnectivityLength += numVertices;
  }

  VTKM_CONT_EXPORT
  void CompleteAddingCells()
  {
    this->PointToCell.Connectivity.Shrink(ConnectivityLength);
    this->PointToCell.ElementsValid = true;
    this->NumberOfCells = this->ConnectivityLength = -1;
  }

  /// Second method to add cells -- all at once.
  /// Copies the data from the vectors, so they can be released.
  VTKM_CONT_EXPORT
  void FillViaCopy(const std::vector<vtkm::Id> &cellTypes,
                   const std::vector<vtkm::Id> &numIndices,
                   const std::vector<vtkm::Id> &connectivity)
  {

    this->PointToCell.Shapes.Allocate( static_cast<vtkm::Id>(cellTypes.size()) );
    std::copy(cellTypes.begin(), cellTypes.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->PointToCell.Shapes.GetPortalControl()));

    this->PointToCell.NumIndices.Allocate( static_cast<vtkm::Id>(numIndices.size()) );
    std::copy(numIndices.begin(), numIndices.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->PointToCell.NumIndices.GetPortalControl()));

    this->PointToCell.Connectivity.Allocate( static_cast<vtkm::Id>(connectivity.size()) );
    std::copy(connectivity.begin(), connectivity.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->PointToCell.Connectivity.GetPortalControl()));

    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = false;
  }

  /// Second method to add cells -- all at once.
  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  void Fill(const vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> &cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::Id, NumIndicesStorageTag> &numIndices,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &connectivity)
  {
    this->PointToCell.Shapes = cellTypes;
    this->PointToCell.NumIndices = numIndices;
    this->PointToCell.Connectivity = connectivity;

    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = false;
  }

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef typename
        ConnectivityChooser<FromTopology,ToTopology>::ConnectivityType
        ContObjectType;

    typedef typename ContObjectType::ShapeArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst ShapePortalType;
    typedef typename ContObjectType::NumIndicesArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst IndicePortalType;
    typedef typename ContObjectType::ConnectivityArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst ConnectivityPortalType;
    typedef typename ContObjectType::IndexOffsetArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst IndexOffsetPortalType;

    typedef vtkm::exec::ConnectivityExplicit<ShapePortalType,
                                             IndicePortalType,
                                             ConnectivityPortalType,
                                             IndexOffsetPortalType
                                             > ExecObjectType;
  };

  template<typename Device, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
  PrepareForInput(Device, FromTopology, ToTopology) const
  {
    const typename
        ConnectivityChooser<FromTopology,ToTopology>::ConnectivityType
        &connectivity = this->GetConnectivity(FromTopology(), ToTopology());

    VTKM_ASSERT_CONT(connectivity.ElementsValid);

    connectivity.BuildIndexOffsets(Device());

    typedef typename
        ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
            ExecObjType;
    return ExecObjType(connectivity.Shapes.PrepareForInput(Device()),
                       connectivity.NumIndices.PrepareForInput(Device()),
                       connectivity.Connectivity.PrepareForInput(Device()),
                       connectivity.IndexOffsets.PrepareForInput(Device()));
  }

  void CreateCellToPointConnectivity()
  {
    std::multimap<vtkm::Id,vtkm::Id> cells_of_nodes;

    vtkm::Id maxNodeID = 0;
    vtkm::Id numCells = GetNumberOfCells();
    vtkm::Id numPoints = GetNumberOfPoints();
    for (vtkm::Id cell = 0, cindex = 0; cell < numCells; ++cell)
    {
      vtkm::Id npts = this->PointToCell.NumIndices.GetPortalControl().Get(cell);
      for (int pt=0; pt<npts; ++pt)
      {
        vtkm::Id index = this->PointToCell.Connectivity.GetPortalControl().Get(cindex++);
        if (index > maxNodeID)
          maxNodeID = index;
        cells_of_nodes.insert(std::pair<vtkm::Id,vtkm::Id>(index,cell));
      }
    }

    std::vector<vtkm::Id> shapes;
    std::vector<vtkm::Id> numindices;
    std::vector<vtkm::Id> conn;

    vtkm::Id filled_array_to_node = 0;
    for (std::multimap<vtkm::Id,vtkm::Id>::iterator iter = cells_of_nodes.begin();
         iter != cells_of_nodes.end(); iter++)
    {
      vtkm::Id node = iter->first;
      while (filled_array_to_node <= node)
      {
        // add empty spots to skip nodes not referenced by our cells
        // also add a spot for the current one
        ++filled_array_to_node;
        shapes.push_back(CELL_SHAPE_VERTEX);
        numindices.push_back(0);
      }
      vtkm::Id cell = iter->second;
      conn.push_back(cell);
      ++numindices[numindices.size()-1];
    }
    while (filled_array_to_node < numPoints)
    {
      // add empty spots for tail nodes not referenced by our cells
      ++filled_array_to_node;
      shapes.push_back(CELL_SHAPE_VERTEX);
      numindices.push_back(0);
    }

    ///\todo: THIS IS A COPY OF "FillViaCopy", because that method
    /// is specific to PointToCell.  Should make it non-specific and call 
    /// it instead.
    this->CellToPoint.Shapes.Allocate( static_cast<vtkm::Id>(shapes.size()) );
    std::copy(shapes.begin(), shapes.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->CellToPoint.Shapes.GetPortalControl()));

    this->CellToPoint.NumIndices.Allocate( static_cast<vtkm::Id>(numindices.size()) );
    std::copy(numindices.begin(), numindices.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->CellToPoint.NumIndices.GetPortalControl()));

    this->CellToPoint.Connectivity.Allocate( static_cast<vtkm::Id>(conn.size()) );
    std::copy(conn.begin(), conn.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->CellToPoint.Connectivity.GetPortalControl()));

    this->CellToPoint.ElementsValid = true;
    this->CellToPoint.IndexOffsetsValid = false;
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out << "   ExplicitCellSet: " << this->Name
          << " dim= " << this->Dimensionality << std::endl;
      out << "   PointToCell: " << std::endl;
      this->PointToCell.PrintSummary(out);
      out << "   CellToPoint: " << std::endl;
      this->CellToPoint.PrintSummary(out);
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> &
  GetShapesArray() const { return this->PointToCell.Shapes; }

  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Id, NumIndicesStorageTag> &
  GetNumIndicesArray() const { return this->PointToCell.NumIndices; }

  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &
  GetConnectivityArray() const { return this->PointToCell.Connectivity; }

  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Id> &
  GetIndexOffsetArray() const {
    return this->PointToCell.IndexOffsets;
  }

private:
  typename ConnectivityChooser<
      vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell>::
      ConnectivityType PointToCell;

  // TODO: Actually implement CellToPoint and other connectivity. (That is,
  // derive the connectivity from PointToCell.
  typename ConnectivityChooser<
      vtkm::TopologyElementTagCell,vtkm::TopologyElementTagPoint>::
      ConnectivityType CellToPoint;

  // A set of overloaded methods to get the connectivity from a pair of
  // topology element types.
#define VTKM_GET_CONNECTIVITY_METHOD(FromTopology,ToTopology,Ivar) \
  VTKM_CONT_EXPORT \
  const typename ConnectivityChooser< \
      FromTopology,ToTopology>::ConnectivityType & \
  GetConnectivity(FromTopology, ToTopology) const \
  { \
    return this->Ivar; \
  } \
  VTKM_CONT_EXPORT \
  typename ConnectivityChooser< \
      FromTopology,ToTopology>::ConnectivityType & \
  GetConnectivity(FromTopology, ToTopology) \
  { \
    return this->Ivar; \
  }

  VTKM_GET_CONNECTIVITY_METHOD(vtkm::TopologyElementTagPoint,
                               vtkm::TopologyElementTagCell,
                               PointToCell)
  VTKM_GET_CONNECTIVITY_METHOD(vtkm::TopologyElementTagCell,
                               vtkm::TopologyElementTagPoint,
                               CellToPoint)

#undef VTKM_GET_CONNECTIVITY_METHOD

  // These are used in the AddCell and related methods to incrementally add
  // cells.
  vtkm::Id ConnectivityLength;
  vtkm::Id NumberOfCells;
  vtkm::Id NumberOfPoints;
};

namespace detail {

template<typename Storage1, typename Storage2, typename Storage3>
struct CellSetExplicitConnectivityChooser<
    vtkm::cont::CellSetExplicit<Storage1,Storage2,Storage3>,
    vtkm::TopologyElementTagPoint,
    vtkm::TopologyElementTagCell>
{
  typedef vtkm::cont::internal::ConnectivityExplicitInternals<
      Storage1,Storage2,Storage3> ConnectivityType;
};

} // namespace detail

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
