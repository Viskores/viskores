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

#include <vtkm/CellShape.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>

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

  static const vtkm::IdComponent NUM_POINTS_IN_CELL = 2;
  static const vtkm::IdComponent MAX_CELL_TO_POINT = 2;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfPoints() const {return this->PointDimensions;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return this->PointDimensions-1;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfPointsInCell() const {return NUM_POINTS_IN_CELL;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetCellShape() const {return vtkm::CELL_SHAPE_LINE;}

  typedef vtkm::CellShapeTagLine CellShapeTag;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,NUM_POINTS_IN_CELL> GetPointsOfCell(vtkm::Id index) const
  {
    vtkm::Vec<vtkm::Id,NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = index;
    pointIds[1] = pointIds[0] + 1;
    return pointIds;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfCellsIncidentOnPoint(vtkm::Id pointIndex) const
  {
    return
        (static_cast<vtkm::IdComponent>(pointIndex > 0)
         + static_cast<vtkm::IdComponent>(pointIndex < this->PointDimensions-1));
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::VecVariable<vtkm::Id,MAX_CELL_TO_POINT>
  GetCellsOfPoint(vtkm::Id index) const
  {
    vtkm::VecVariable<vtkm::Id,MAX_CELL_TO_POINT> cellIds;

    if (index > 0)
    {
      cellIds.Append(index-1);
    }
    if (index < this->PointDimensions-1)
    {
      cellIds.Append(index);
    }

    return cellIds;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const
  {
    return flatPointIndex;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatPointIndex(vtkm::Id logicalPointIndex) const
  {
    return logicalPointIndex;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const
  {
    return flatCellIndex;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatCellIndex(vtkm::Id logicalCellIndex) const
  {
    return logicalCellIndex;
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

  static const vtkm::IdComponent NUM_POINTS_IN_CELL = 4;
  static const vtkm::IdComponent MAX_CELL_TO_POINT = 4;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return vtkm::internal::VecProduct<2>()(this->GetCellDimensions());
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfPointsInCell() const {return NUM_POINTS_IN_CELL;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetCellShape() const { return vtkm::CELL_SHAPE_QUAD; }

  typedef vtkm::CellShapeTagQuad CellShapeTag;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,NUM_POINTS_IN_CELL>
      GetPointsOfCell(vtkm::Id cellIndex) const
  {
    vtkm::Id2 ij = this->FlatToLogicalCellIndex(cellIndex);

    vtkm::Vec<vtkm::Id,NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = this->LogicalToFlatPointIndex(ij);
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    return pointIds;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfCellsIncidentOnPoint(vtkm::Id pointIndex) const
  {
    vtkm::Id2 ij = this->FlatToLogicalPointIndex(pointIndex);
    return
        (static_cast<vtkm::IdComponent>((ij[0] > 0) && (ij[1] > 0))
         + static_cast<vtkm::IdComponent>((ij[0] < this->PointDimensions[0]-1) && (ij[1] > 0))
         + static_cast<vtkm::IdComponent>((ij[0] > 0) && (ij[1] < this->PointDimensions[1]-1))
         + static_cast<vtkm::IdComponent>(
          (ij[0] < this->PointDimensions[0]-1) && (ij[1] < this->PointDimensions[1]-1)));
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::VecVariable<vtkm::Id,MAX_CELL_TO_POINT>
  GetCellsOfPoint(vtkm::Id pointIndex) const
  {
    vtkm::VecVariable<vtkm::Id,MAX_CELL_TO_POINT> cellIds;

    vtkm::Id2 ij = this->FlatToLogicalPointIndex(pointIndex);
    if ((ij[0] > 0) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - vtkm::Id2(1,1)));
    }
    if ((ij[0] < this->PointDimensions[0]-1) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - vtkm::Id2(0,1)));
    }
    if ((ij[0] > 0) && (ij[1] < this->PointDimensions[1]-1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - vtkm::Id2(1,0)));
    }
    if ((ij[0] < this->PointDimensions[0]-1) && (ij[1] < this->PointDimensions[1]-1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij));
    }

    return cellIds;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id2 FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const
  {
    vtkm::Id2 logicalPointIndex;
    logicalPointIndex[0] = flatPointIndex % this->PointDimensions[0];
    logicalPointIndex[1] = flatPointIndex / this->PointDimensions[0];
    return logicalPointIndex;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatPointIndex(const vtkm::Id2 &logicalPointIndex) const
  {
    return logicalPointIndex[0]
        + this->PointDimensions[0]*logicalPointIndex[1];
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id2 FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const
  {
    vtkm::Id2 cellDimensions = this->GetCellDimensions();
    vtkm::Id2 logicalCellIndex;
    logicalCellIndex[0] = flatCellIndex % cellDimensions[0];
    logicalCellIndex[1] = flatCellIndex / cellDimensions[0];
    return logicalCellIndex;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatCellIndex(const vtkm::Id2 &logicalCellIndex) const
  {
    vtkm::Id2 cellDimensions = this->GetCellDimensions();
    return logicalCellIndex[0]
        + cellDimensions[0]*logicalCellIndex[1];
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
    this->CellDimensions = dims - vtkm::Id3(1);
    this->CellDim01 = (dims[0]-1) * (dims[1]-1);
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

  static const vtkm::IdComponent NUM_POINTS_IN_CELL = 8;
  static const vtkm::IdComponent MAX_CELL_TO_POINT = 6;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return vtkm::internal::VecProduct<3>()(this->GetCellDimensions());
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfPointsInCell() const {return NUM_POINTS_IN_CELL;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetCellShape() const { return vtkm::CELL_SHAPE_HEXAHEDRON; }

  typedef vtkm::CellShapeTagHexahedron CellShapeTag;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,NUM_POINTS_IN_CELL> GetPointsOfCell(vtkm::Id cellIndex) const
  {
    vtkm::Id3 ijk = this->FlatToLogicalCellIndex(cellIndex);

    vtkm::Vec<vtkm::Id,NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = (ijk[2] * this->PointDimensions[1] + ijk[1]) * this->PointDimensions[0] + ijk[0];
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    pointIds[4] = pointIds[0] + this->PointDimensions[0]*this->PointDimensions[1];
    pointIds[5] = pointIds[4] + 1;
    pointIds[6] = pointIds[5] + this->PointDimensions[0];
    pointIds[7] = pointIds[6] - 1;

    return pointIds;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfCellsIncidentOnPoint(vtkm::Id pointIndex) const
  {
    vtkm::Id3 ijk = this->FlatToLogicalPointIndex(pointIndex);

    return (
            static_cast<vtkm::IdComponent>((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] > 0))
          + static_cast<vtkm::IdComponent>((ijk[0] < this->PointDimensions[0]-1) && (ijk[1] > 0) && (ijk[2] > 0))
          + static_cast<vtkm::IdComponent>((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1]-1) && (ijk[2] > 0))
          + static_cast<vtkm::IdComponent>((ijk[0] < this->PointDimensions[0]-1) &&
                                           (ijk[1] < this->PointDimensions[1]-1) &&
                                           (ijk[2] > 0))
          + static_cast<vtkm::IdComponent>((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] < this->PointDimensions[2]-1))
          + static_cast<vtkm::IdComponent>((ijk[0] < this->PointDimensions[0]-1) &&
                                           (ijk[1] > 0) &&
                                           (ijk[2] < this->PointDimensions[2]-1))
          + static_cast<vtkm::IdComponent>((ijk[0] > 0) &&
                                           (ijk[1] < this->PointDimensions[1]-1) &&
                                           (ijk[2] < this->PointDimensions[2]-1))
          + static_cast<vtkm::IdComponent>((ijk[0] < this->PointDimensions[0]-1) &&
                                           (ijk[1] < this->PointDimensions[1]-1) &&
                                           (ijk[2] < this->PointDimensions[2]-1))
            );
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::VecVariable<vtkm::Id,MAX_CELL_TO_POINT>
  GetCellsOfPoint(vtkm::Id pointIndex) const
  {
    vtkm::VecVariable<vtkm::Id,MAX_CELL_TO_POINT> cellIds;

    vtkm::Id3 ijk = this->FlatToLogicalPointIndex(pointIndex);

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1,1,1)));
    }
    if ((ijk[0] < this->PointDimensions[0]-1) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(0,1,1)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1]-1) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1,0,1)));
    }
    if ((ijk[0] < this->PointDimensions[0]-1) &&
        (ijk[1] < this->PointDimensions[1]-1) &&
        (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(0,0,1)));
    }

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] < this->PointDimensions[2]-1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1,1,0)));
    }
    if ((ijk[0] < this->PointDimensions[0]-1) &&
        (ijk[1] > 0) &&
        (ijk[2] < this->PointDimensions[2]-1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(0,1,0)));
    }
    if ((ijk[0] > 0) &&
        (ijk[1] < this->PointDimensions[1]-1) &&
        (ijk[2] < this->PointDimensions[2]-1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1,0,0)));
    }
    if ((ijk[0] < this->PointDimensions[0]-1) &&
        (ijk[1] < this->PointDimensions[1]-1) &&
        (ijk[2] < this->PointDimensions[2]-1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk));
    }

    return cellIds;
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
    out<<"   RegularConnectivity<3> ";
    out<<"pointDim["<<this->PointDimensions[0]<<" "<<this->PointDimensions[1]<<" "<<this->PointDimensions[2]<<"] ";
    out<<std::endl;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const
  {
    const vtkm::Id pointDims01 = this->PointDimensions[0] * this->PointDimensions[1];
    const vtkm::Id indexij = flatPointIndex % pointDims01;

    return vtkm::Id3(indexij % this->PointDimensions[0],
                     indexij / this->PointDimensions[0],
                     flatPointIndex / pointDims01);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatPointIndex(const vtkm::Id3 &logicalPointIndex) const
  {
    return logicalPointIndex[0]
        + this->PointDimensions[0]*(logicalPointIndex[1]
                                    + this->PointDimensions[1]*logicalPointIndex[2]);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const
  {
    const vtkm::Id indexij = flatCellIndex % this->CellDim01;
    return vtkm::Id3(indexij % this->CellDimensions[0],
                     indexij / this->CellDimensions[0],
                     flatCellIndex / this->CellDim01
                     );
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id LogicalToFlatCellIndex(const vtkm::Id3 &logicalCellIndex) const
  {
    vtkm::Id3 cellDimensions = this->GetCellDimensions();
    return logicalCellIndex[0]
        + cellDimensions[0]*(logicalCellIndex[1]
                                    + cellDimensions[1]*logicalCellIndex[2]);
  }

private:
  vtkm::Id3 PointDimensions;
  vtkm::Id3 CellDimensions;
  vtkm::Id  CellDim01;
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
  VTKM_STATIC_ASSERT_MSG(sizeof(To) == static_cast<size_t>(-1),
                         "Missing Specialization for Topologies");
};

template<vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<
    vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, Dimension>
{
  typedef vtkm::internal::ConnectivityStructuredInternals<Dimension>
      ConnectivityType;

  typedef vtkm::Vec<vtkm::Id,ConnectivityType::NUM_POINTS_IN_CELL> IndicesType;

  VTKM_EXEC_CONT_EXPORT
  static IndicesType GetIndices(const ConnectivityType &connectivity,
                                vtkm::Id cellIndex)
  {
    return connectivity.GetPointsOfCell(cellIndex);
  }

  VTKM_EXEC_CONT_EXPORT
  static vtkm::IdComponent GetNumberOfIndices(
        const ConnectivityType &connectivity,
        vtkm::Id vtkmNotUsed(cellIndex))
  {
    return connectivity.GetNumberOfPointsInCell();
  }
};

template<vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<
    vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, Dimension>
{
  typedef vtkm::internal::ConnectivityStructuredInternals<Dimension>
      ConnectivityType;

  // TODO: This needs to change to a Vec-like that supports a max size.
  // Likewise, all the GetCellsOfPoint methods need to use it as well.
  typedef vtkm::VecVariable<vtkm::Id,ConnectivityType::MAX_CELL_TO_POINT>
      IndicesType;

  VTKM_EXEC_CONT_EXPORT
  static IndicesType GetIndices(const ConnectivityType &connectivity,
                                vtkm::Id pointIndex)
  {
    return connectivity.GetCellsOfPoint(pointIndex);
  }

  VTKM_EXEC_CONT_EXPORT
  static vtkm::IdComponent GetNumberOfIndices(
        const ConnectivityType &connectivity,
        vtkm::Id pointIndex)
  {
    return connectivity.GetNumberOfCellsIncidentOnPoint(pointIndex);
  }
};

}
} // namespace vtkm::internal

#endif //vtk_m_internal_ConnectivityStructuredInternals_h
