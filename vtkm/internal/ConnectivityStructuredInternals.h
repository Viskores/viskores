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

#ifndef vtk_m_internal_ConnectivityStructuredInternals_h
#define vtk_m_internal_ConnectivityStructuredInternals_h

#include <vtkm/CellType.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>

VTKM_BOOST_PRE_INCLUDE
#include <boost/static_assert.hpp>
VTKM_BOOST_POST_INCLUDE

#include <iostream>

namespace vtkm {
namespace internal {

template<vtkm::IdComponent> class ConnectivityStructuredInternals;

//1 D specialization.
template<>
class ConnectivityStructuredInternals<1>
{
public:
  typedef vtkm::Id SchedulingRangeType;

  VTKM_EXEC_CONT_EXPORT
  void SetPointDimensions(vtkm::Id dimensions)
  {
    this->PointDimensions = dimensions;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetPointDimensions() const
  {
    return this->PointDimensions;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetCellDimensions() const
  {
    return this->PointDimensions - 1;
  }

  VTKM_EXEC_CONT_EXPORT
  SchedulingRangeType GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  VTKM_EXEC_CONT_EXPORT
  SchedulingRangeType GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfPoints() const {return this->PointDimensions;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return this->PointDimensions-1;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfNodesPerCell() const {return 2;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetCellShapeType() const {return VTKM_LINE;}

  template <vtkm::IdComponent IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetPointsOfCell(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 2);
    ids[0] = index;
    ids[1] = ids[0] + 1;
  }

  template <vtkm::IdComponent IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfPoint(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 2);
    ids[0] = ids[1] = -1;
    vtkm::IdComponent idx = 0;
    if (index > 0)
    {
      ids[idx++] = index-1;
    }
    if (index < this->PointDimensions-1)
    {
      ids[idx++] = index;
    }
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
    out<<"   RegularConnectivity<1> ";
    out<<"this->PointDimensions["<<this->PointDimensions<<"] ";
    out<<"\n";
  }

private:
  vtkm::Id PointDimensions;
};

//2 D specialization.
template<>
class ConnectivityStructuredInternals<2>
{
public:
  typedef vtkm::Id2 SchedulingRangeType;

  VTKM_EXEC_CONT_EXPORT
  void SetPointDimensions(vtkm::Id2 dims)
  {
    this->PointDimensions = dims;
  }

  VTKM_EXEC_CONT_EXPORT
  const vtkm::Id2 &GetPointDimensions() const {
    return this->PointDimensions;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id2 GetCellDimensions() const {
    return this->PointDimensions - vtkm::Id2(1);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfPoints() const {
    return vtkm::internal::VecProduct<2>()(this->GetPointDimensions());
  }

  //returns an id2 to signal what kind of scheduling to use
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagCell) const {
    return this->GetCellDimensions();
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagPoint) const {
    return this->GetPointDimensions();
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return vtkm::internal::VecProduct<2>()(this->GetCellDimensions());
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfNodesPerCell() const { return 4; }
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetCellShapeType() const { return VTKM_PIXEL; }

  template <vtkm::IdComponent IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetPointsOfCell(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 4);

    vtkm::Id i, j;
    this->CalculateLogicalNodeIndices(index, i, j);

    ids[0] = j*this->PointDimensions[0] + i;
    ids[1] = ids[0] + 1;
    ids[2] = ids[0] + this->PointDimensions[0];
    ids[3] = ids[2] + 1;
  }

  template <vtkm::IdComponent IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfPoint(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 4);

    ids[0] = ids[1] = ids[2] = ids[3] = -1;
    vtkm::Id i, j;
    vtkm::IdComponent idx = 0;
    CalculateLogicalNodeIndices(index, i, j);
    if ((i > 0) && (j > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j-1);
    }
    if ((i < this->PointDimensions[0]-1) && (j > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j-1);
    }
    if ((i > 0) && (j < this->PointDimensions[1]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j  );
    }
    if ((i < this->PointDimensions[0]-1) && (j < this->PointDimensions[1]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j  );
    }
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
      out<<"   RegularConnectivity<2> ";
      out<<"pointDim["<<this->PointDimensions[0]<<" "<<this->PointDimensions[1]<<"] ";
      out<<std::endl;
  }

private:
  vtkm::Id2 PointDimensions;

private:
  VTKM_EXEC_CONT_EXPORT
  void CalculateLogicalNodeIndices(vtkm::Id index, vtkm::Id &i, vtkm::Id &j) const
  {
    vtkm::Id2 cellDimensions = this->GetCellDimensions();
    i = index % cellDimensions[0];
    j = index / cellDimensions[0];
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id CalculateCellIndex(vtkm::Id i, vtkm::Id j) const
  {
    vtkm::Id2 cellDimensions = this->GetCellDimensions();
    return j*cellDimensions[0] + i;
  }
};

//3 D specialization.
template<>
class ConnectivityStructuredInternals<3>
{
public:
  typedef vtkm::Id3 SchedulingRangeType;

  VTKM_EXEC_CONT_EXPORT
  void SetPointDimensions(vtkm::Id3 dims)
  {
    this->PointDimensions = dims;
  }

  VTKM_EXEC_CONT_EXPORT
  const vtkm::Id3 GetPointDimensions() const
  {
    return this->PointDimensions;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 GetCellDimensions() const
  {
    return this->PointDimensions - vtkm::Id3(1);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfPoints() const
  {
    return vtkm::internal::VecProduct<3>()(this->PointDimensions);
  }

  //returns an id3 to signal what kind of scheduling to use
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 GetSchedulingRange(vtkm::TopologyElementTagCell) const {
    return this->GetCellDimensions();
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 GetSchedulingRange(vtkm::TopologyElementTagPoint) const {
    return this->GetPointDimensions();
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return vtkm::internal::VecProduct<3>()(this->GetCellDimensions());
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfNodesPerCell() const { return 8; }
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetCellShapeType() const { return VTKM_VOXEL; }

  template <vtkm::IdComponent IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetPointsOfCell(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 8);

    vtkm::Id3 cellDimensions = this->GetCellDimensions();

    vtkm::Id cellDims01 = cellDimensions[0] * cellDimensions[1];
    vtkm::Id k = index / cellDims01;
    vtkm::Id indexij = index % cellDims01;
    vtkm::Id j = indexij / cellDimensions[0];
    vtkm::Id i = indexij % cellDimensions[0];

    ids[0] = (k * this->PointDimensions[1] + j) * this->PointDimensions[0] + i;
    ids[1] = ids[0] + 1;
    ids[2] = ids[0] + this->PointDimensions[0];
    ids[3] = ids[2] + 1;
    ids[4] = ids[0] + this->PointDimensions[0]*this->PointDimensions[1];
    ids[5] = ids[4] + 1;
    ids[6] = ids[4] + this->PointDimensions[0];
    ids[7] = ids[6] + 1;
  }

  template <vtkm::IdComponent IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfPoint(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 8);

    ids[0]=ids[1]=ids[2]=ids[3]=ids[4]=ids[5]=ids[6]=ids[7]=-1;

    vtkm::Id i, j, k;
    vtkm::IdComponent idx=0;

    this->CalculateLogicalNodeIndices(index, i, j, k);
    if ((i > 0) && (j > 0) && (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j-1, k-1);
    }
    if ((i < this->PointDimensions[0]-1) && (j > 0) && (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j-1, k-1);
    }
    if ((i > 0) && (j < this->PointDimensions[1]-1) && (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j  , k-1);
    }
    if ((i < this->PointDimensions[0]-1) &&
        (j < this->PointDimensions[1]-1) &&
        (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j  , k-1);
    }

    if ((i > 0) && (j > 0) && (k < this->PointDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j-1, k);
    }
    if ((i < this->PointDimensions[0]-1) &&
        (j > 0) &&
        (k < this->PointDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j-1, k);
    }
    if ((i > 0) &&
        (j < this->PointDimensions[1]-1) &&
        (k < this->PointDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j  , k);
    }
    if ((i < this->PointDimensions[0]-1) &&
        (j < this->PointDimensions[1]-1) &&
        (k < this->PointDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j  , k);
    }
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
    out<<"   RegularConnectivity<3> ";
    out<<"pointDim["<<this->PointDimensions[0]<<" "<<this->PointDimensions[1]<<" "<<this->PointDimensions[2]<<"] ";
    out<<std::endl;
  }

private:
  vtkm::Id3 PointDimensions;


private:
  VTKM_EXEC_CONT_EXPORT
  void CalculateLogicalNodeIndices(vtkm::Id index, vtkm::Id &i, vtkm::Id &j, vtkm::Id &k) const
  {
    vtkm::Id nodeDims01 = this->PointDimensions[0] * this->PointDimensions[1];
    k = index / nodeDims01;
    vtkm::Id indexij = index % nodeDims01;
    j = indexij / this->PointDimensions[0];
    i = indexij % this->PointDimensions[0];
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id CalculateCellIndex(vtkm::Id i, vtkm::Id j, vtkm::Id k) const
  {
    vtkm::Id3 cellDimensions = this->GetCellDimensions();
    return (k * cellDimensions[1] + j) * cellDimensions[0] + i;
  }
};

// We may want to generalize this class depending on how ConnectivityExplicit
// eventually handles retrieving cell to point connectivity.

template<typename From, typename To, vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper
{
  // We want an unconditional failure if this unspecialized class ever gets
  // instantiated, because it means someone missed a topology mapping type.
  // We need to create a test which depends on the templated types so
  // it doesn't get picked up without a concrete instantiation.
  BOOST_STATIC_ASSERT_MSG(sizeof(To) == static_cast<size_t>(-1),
                          "Missing Specialization for Topologies");
};

template<vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<
    vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, Dimension>
{
  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_CONT_EXPORT
  static void GetIndices(
      const vtkm::internal::ConnectivityStructuredInternals<Dimension> &connectivity,
      vtkm::Id cellIndex,
      vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    connectivity.GetPointsOfCell(cellIndex, ids);
  }

  VTKM_EXEC_CONT_EXPORT
  static vtkm::IdComponent GetNumberOfIndices(
        const vtkm::internal::ConnectivityStructuredInternals<Dimension> &connectivity,
        vtkm::Id vtkmNotUsed(cellIndex))
  {
    return connectivity.GetNumberOfNodesPerCell();
  }
};

template<vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<
    vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, Dimension>
{
  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_CONT_EXPORT
  static void GetIndices(
      const vtkm::internal::ConnectivityStructuredInternals<Dimension> &connectivity,
      vtkm::Id pointIndex,
      vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    connectivity.GetCellsOfPoint(pointIndex,ids);
  }

  // TODO: Implement GetNumberOfIndices, which will rely on a
  // GetNumberOfCellsOnPoint method in ConnectivityStructuredInternals
};

}
} // namespace vtkm::internal

#endif //vtk_m_internal_ConnectivityStructuredInternals_h
