//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_internal_ConnectivityStructuredInternals_h
#define vtk_m_internal_ConnectivityStructuredInternals_h

#include <vtkm/CellShape.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>

#include <vtkm/internal/Assume.h>

namespace vtkm
{
namespace internal
{

template <vtkm::IdComponent>
class ConnectivityStructuredInternals;

//1 D specialization.
template <>
class ConnectivityStructuredInternals<1>
{
public:
  using SchedulingRangeType = vtkm::Id;

  VTKM_EXEC_CONT
  void SetPointDimensions(vtkm::Id dimensions) { this->PointDimensions = dimensions; }

  VTKM_EXEC_CONT
  void SetGlobalPointIndexStart(vtkm::Id start) { this->GlobalPointIndexStart = start; }

  VTKM_EXEC_CONT
  vtkm::Id GetPointDimensions() const { return this->PointDimensions; }

  VTKM_EXEC_CONT
  vtkm::Id GetCellDimensions() const { return this->PointDimensions - 1; }

  VTKM_EXEC_CONT
  SchedulingRangeType GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  VTKM_EXEC_CONT
  SchedulingRangeType GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  VTKM_EXEC_CONT
  SchedulingRangeType GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr vtkm::IdComponent NUM_POINTS_IN_CELL = 2;
  static constexpr vtkm::IdComponent MAX_CELL_TO_POINT = 2;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfPoints() const { return this->PointDimensions; }
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfCells() const { return this->PointDimensions - 1; }
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  VTKM_EXEC_CONT
  vtkm::IdComponent GetCellShape() const { return vtkm::CELL_SHAPE_LINE; }

  using CellShapeTag = vtkm::CellShapeTagLine;

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(vtkm::Id index) const
  {
    VTKM_ASSUME(index >= 0);

    vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = index;
    pointIds[1] = pointIds[0] + 1;
    return pointIds;
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfCellsOnPoint(vtkm::Id pointIndex) const
  {
    VTKM_ASSUME(pointIndex >= 0);

    if ((pointIndex > 0) && (pointIndex < this->PointDimensions - 1))
    {
      return 2;
    }
    else
    {
      return 1;
    }
  }

  VTKM_EXEC_CONT
  vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(vtkm::Id index) const
  {
    VTKM_ASSUME(index >= 0);
    VTKM_ASSUME(this->PointDimensions > 1);

    vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> cellIds;

    if (index > 0)
    {
      cellIds.Append(index - 1);
    }
    if (index < this->PointDimensions - 1)
    {
      cellIds.Append(index);
    }

    return cellIds;
  }


  VTKM_EXEC_CONT
  vtkm::Id FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const { return flatPointIndex; }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatPointIndex(vtkm::Id logicalPointIndex) const { return logicalPointIndex; }

  VTKM_EXEC_CONT
  vtkm::Id FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const { return flatCellIndex; }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatCellIndex(vtkm::Id logicalCellIndex) const { return logicalCellIndex; }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<1> ";
    out << "this->PointDimensions[" << this->PointDimensions << "] ";
    out << "\n";
  }

private:
  vtkm::Id PointDimensions = 0;
  vtkm::Id GlobalPointIndexStart = 0;
};

//2 D specialization.
template <>
class ConnectivityStructuredInternals<2>
{
public:
  using SchedulingRangeType = vtkm::Id2;

  VTKM_EXEC_CONT
  void SetPointDimensions(vtkm::Id2 dims) { this->PointDimensions = dims; }

  VTKM_EXEC_CONT
  void SetGlobalPointIndexStart(vtkm::Id2 start) { this->GlobalPointIndexStart = start; }

  VTKM_EXEC_CONT
  const vtkm::Id2& GetPointDimensions() const { return this->PointDimensions; }

  VTKM_EXEC_CONT
  vtkm::Id2 GetCellDimensions() const { return this->PointDimensions - vtkm::Id2(1); }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfPoints() const { return vtkm::ReduceProduct(this->GetPointDimensions()); }

  //returns an id2 to signal what kind of scheduling to use
  VTKM_EXEC_CONT
  vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetCellDimensions();
  }
  VTKM_EXEC_CONT
  vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetPointDimensions();
  }

  VTKM_EXEC_CONT
  const vtkm::Id2& GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr vtkm::IdComponent NUM_POINTS_IN_CELL = 4;
  static constexpr vtkm::IdComponent MAX_CELL_TO_POINT = 4;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfCells() const { return vtkm::ReduceProduct(this->GetCellDimensions()); }
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  VTKM_EXEC_CONT
  vtkm::IdComponent GetCellShape() const { return vtkm::CELL_SHAPE_QUAD; }

  using CellShapeTag = vtkm::CellShapeTagQuad;

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(
    const SchedulingRangeType& logicalCellIndex) const
  {
    vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = this->LogicalToFlatPointIndex(logicalCellIndex);
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    return pointIds;
  }

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(vtkm::Id cellIndex) const
  {
    return this->GetPointsOfCell(this->FlatToLogicalCellIndex(cellIndex));
  }

  VTKM_EXEC_CONT vtkm::IdComponent GetNumberOfCellsOnPoint(const SchedulingRangeType& ij) const
  {
    vtkm::IdComponent numCells = 1;

    for (vtkm::IdComponent dim = 0; dim < 2; dim++)
    {
      if ((ij[dim] > 0) && (ij[dim] < this->PointDimensions[dim] - 1))
      {
        numCells *= 2;
      }
    }

    return numCells;
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfCellsOnPoint(vtkm::Id pointIndex) const
  {
    return this->GetNumberOfCellsOnPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VTKM_EXEC_CONT
  vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    const SchedulingRangeType& ij) const
  {
    vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> cellIds;

    if ((ij[0] > 0) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - vtkm::Id2(1, 1)));
    }
    if ((ij[0] < this->PointDimensions[0] - 1) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - vtkm::Id2(0, 1)));
    }
    if ((ij[0] > 0) && (ij[1] < this->PointDimensions[1] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - vtkm::Id2(1, 0)));
    }
    if ((ij[0] < this->PointDimensions[0] - 1) && (ij[1] < this->PointDimensions[1] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij));
    }

    return cellIds;
  }

  VTKM_EXEC_CONT
  vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(vtkm::Id pointIndex) const
  {
    return this->GetCellsOfPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VTKM_EXEC_CONT
  vtkm::Id2 FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const
  {
    vtkm::Id2 logicalPointIndex;
    logicalPointIndex[0] = flatPointIndex % this->PointDimensions[0];
    logicalPointIndex[1] = flatPointIndex / this->PointDimensions[0];
    return logicalPointIndex;
  }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatPointIndex(const vtkm::Id2& logicalPointIndex) const
  {
    return logicalPointIndex[0] + this->PointDimensions[0] * logicalPointIndex[1];
  }

  VTKM_EXEC_CONT
  vtkm::Id2 FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const
  {
    vtkm::Id2 cellDimensions = this->GetCellDimensions();
    vtkm::Id2 logicalCellIndex;
    logicalCellIndex[0] = flatCellIndex % cellDimensions[0];
    logicalCellIndex[1] = flatCellIndex / cellDimensions[0];
    return logicalCellIndex;
  }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatCellIndex(const vtkm::Id2& logicalCellIndex) const
  {
    vtkm::Id2 cellDimensions = this->GetCellDimensions();
    return logicalCellIndex[0] + cellDimensions[0] * logicalCellIndex[1];
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<2> ";
    out << "pointDim[" << this->PointDimensions[0] << " " << this->PointDimensions[1] << "] ";
    out << std::endl;
  }

private:
  vtkm::Id2 PointDimensions = { 0, 0 };
  vtkm::Id2 GlobalPointIndexStart = { 0, 0 };
};

//3 D specialization.
template <>
class ConnectivityStructuredInternals<3>
{
public:
  using SchedulingRangeType = vtkm::Id3;

  VTKM_EXEC_CONT
  void SetPointDimensions(vtkm::Id3 dims)
  {
    this->PointDimensions = dims;
    this->CellDimensions = dims - vtkm::Id3(1);
    this->CellDim01 = (dims[0] - 1) * (dims[1] - 1);
  }

  VTKM_EXEC_CONT
  void SetGlobalPointIndexStart(vtkm::Id3 start) { this->GlobalPointIndexStart = start; }

  VTKM_EXEC_CONT
  const vtkm::Id3& GetPointDimensions() const { return this->PointDimensions; }

  VTKM_EXEC_CONT
  const vtkm::Id3& GetCellDimensions() const { return this->CellDimensions; }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfPoints() const { return vtkm::ReduceProduct(this->PointDimensions); }

  //returns an id3 to signal what kind of scheduling to use
  VTKM_EXEC_CONT
  const vtkm::Id3& GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetCellDimensions();
  }
  VTKM_EXEC_CONT
  const vtkm::Id3& GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetPointDimensions();
  }

  VTKM_EXEC_CONT
  const vtkm::Id3& GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr vtkm::IdComponent NUM_POINTS_IN_CELL = 8;
  static constexpr vtkm::IdComponent MAX_CELL_TO_POINT = 8;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfCells() const { return vtkm::ReduceProduct(this->GetCellDimensions()); }
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  VTKM_EXEC_CONT
  vtkm::IdComponent GetCellShape() const { return vtkm::CELL_SHAPE_HEXAHEDRON; }

  using CellShapeTag = vtkm::CellShapeTagHexahedron;

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(const SchedulingRangeType& ijk) const
  {
    vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = (ijk[2] * this->PointDimensions[1] + ijk[1]) * this->PointDimensions[0] + ijk[0];
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    pointIds[4] = pointIds[0] + this->PointDimensions[0] * this->PointDimensions[1];
    pointIds[5] = pointIds[4] + 1;
    pointIds[6] = pointIds[5] + this->PointDimensions[0];
    pointIds[7] = pointIds[6] - 1;

    return pointIds;
  }

  VTKM_EXEC_CONT
  vtkm::Vec<vtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(vtkm::Id cellIndex) const
  {
    return this->GetPointsOfCell(this->FlatToLogicalCellIndex(cellIndex));
  }

  VTKM_EXEC_CONT vtkm::IdComponent GetNumberOfCellsOnPoint(const SchedulingRangeType& ijk) const
  {
    vtkm::IdComponent numCells = 1;

    for (vtkm::IdComponent dim = 0; dim < 3; dim++)
    {
      if ((ijk[dim] > 0) && (ijk[dim] < this->PointDimensions[dim] - 1))
      {
        numCells *= 2;
      }
    }

    return numCells;
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfCellsOnPoint(vtkm::Id pointIndex) const
  {
    return this->GetNumberOfCellsOnPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VTKM_EXEC_CONT
  vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    const SchedulingRangeType& ijk) const
  {
    vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> cellIds;

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1, 1, 1)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(0, 1, 1)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1] - 1) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1, 0, 1)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(0, 0, 1)));
    }

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1, 1, 0)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] > 0) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(0, 1, 0)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - vtkm::Id3(1, 0, 0)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk));
    }

    return cellIds;
  }

  VTKM_EXEC_CONT
  vtkm::VecVariable<vtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(vtkm::Id pointIndex) const
  {
    return this->GetCellsOfPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<3> ";
    out << "pointDim[" << this->PointDimensions[0] << " " << this->PointDimensions[1] << " "
        << this->PointDimensions[2] << "] ";
    out << std::endl;
  }

  VTKM_EXEC_CONT
  vtkm::Id3 FlatToLogicalPointIndex(vtkm::Id flatPointIndex) const
  {
    const vtkm::Id pointDims01 = this->PointDimensions[0] * this->PointDimensions[1];
    const vtkm::Id indexij = flatPointIndex % pointDims01;

    return vtkm::Id3(indexij % this->PointDimensions[0],
                     indexij / this->PointDimensions[0],
                     flatPointIndex / pointDims01);
  }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatPointIndex(const vtkm::Id3& logicalPointIndex) const
  {
    return logicalPointIndex[0] +
      this->PointDimensions[0] *
      (logicalPointIndex[1] + this->PointDimensions[1] * logicalPointIndex[2]);
  }

  VTKM_EXEC_CONT
  vtkm::Id3 FlatToLogicalCellIndex(vtkm::Id flatCellIndex) const
  {
    const vtkm::Id indexij = flatCellIndex % this->CellDim01;
    return vtkm::Id3(indexij % this->CellDimensions[0],
                     indexij / this->CellDimensions[0],
                     flatCellIndex / this->CellDim01);
  }

  VTKM_EXEC_CONT
  vtkm::Id LogicalToFlatCellIndex(const vtkm::Id3& logicalCellIndex) const
  {
    return logicalCellIndex[0] +
      this->CellDimensions[0] *
      (logicalCellIndex[1] + this->CellDimensions[1] * logicalCellIndex[2]);
  }

private:
  vtkm::Id3 PointDimensions = { 0, 0, 0 };
  vtkm::Id3 GlobalPointIndexStart = { 0, 0, 0 };
  vtkm::Id3 CellDimensions = { 0, 0, 0 };
  vtkm::Id CellDim01 = 0;
};

// We may want to generalize this class depending on how ConnectivityExplicit
// eventually handles retrieving cell to point connectivity.

template <typename VisitTopology, typename IncidentTopology, vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper
{
  // We want an unconditional failure if this unspecialized class ever gets
  // instantiated, because it means someone missed a topology mapping type.
  // We need to create a test which depends on the templated types so
  // it doesn't get picked up without a concrete instantiation.
  VTKM_STATIC_ASSERT_MSG(sizeof(VisitTopology) == static_cast<size_t>(-1),
                         "Missing Specialization for Topologies");
};

template <vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<vtkm::TopologyElementTagCell,
                                         vtkm::TopologyElementTagPoint,
                                         Dimension>
{
  using ConnectivityType = vtkm::internal::ConnectivityStructuredInternals<Dimension>;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  using CellShapeTag = typename ConnectivityType::CellShapeTag;

  using IndicesType = vtkm::Vec<vtkm::Id, ConnectivityType::NUM_POINTS_IN_CELL>;

  VTKM_EXEC_CONT static vtkm::Id GetNumberOfElements(const ConnectivityType& connectivity)
  {
    return connectivity.GetNumberOfCells();
  }

  template <typename IndexType>
  VTKM_EXEC_CONT static vtkm::IdComponent GetNumberOfIndices(
    const ConnectivityType& vtkmNotUsed(connectivity),
    const IndexType& vtkmNotUsed(cellIndex))
  {
    return ConnectivityType::NUM_POINTS_IN_CELL;
  }

  template <typename IndexType>
  VTKM_EXEC_CONT static IndicesType GetIndices(const ConnectivityType& connectivity,
                                               const IndexType& cellIndex)
  {
    return connectivity.GetPointsOfCell(cellIndex);
  }

  VTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalFromIndex(const ConnectivityType& connectivity,
                                                 vtkm::Id flatFromIndex)
  {
    return connectivity.FlatToLogicalPointIndex(flatFromIndex);
  }

  VTKM_EXEC_CONT
  static vtkm::Id LogicalToFlatFromIndex(const ConnectivityType& connectivity,
                                         const LogicalIndexType& logicalFromIndex)
  {
    return connectivity.LogicalToFlatPointIndex(logicalFromIndex);
  }

  VTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalToIndex(const ConnectivityType& connectivity,
                                               vtkm::Id flatToIndex)
  {
    return connectivity.FlatToLogicalCellIndex(flatToIndex);
  }

  VTKM_EXEC_CONT
  static vtkm::Id LogicalToFlatToIndex(const ConnectivityType& connectivity,
                                       const LogicalIndexType& logicalToIndex)
  {
    return connectivity.LogicalToFlatCellIndex(logicalToIndex);
  }
};

template <vtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<vtkm::TopologyElementTagPoint,
                                         vtkm::TopologyElementTagCell,
                                         Dimension>
{
  using ConnectivityType = vtkm::internal::ConnectivityStructuredInternals<Dimension>;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  using CellShapeTag = vtkm::CellShapeTagVertex;

  using IndicesType = vtkm::VecVariable<vtkm::Id, ConnectivityType::MAX_CELL_TO_POINT>;

  VTKM_EXEC_CONT static vtkm::Id GetNumberOfElements(const ConnectivityType& connectivity)
  {
    return connectivity.GetNumberOfPoints();
  }

  template <typename IndexType>
  VTKM_EXEC_CONT static vtkm::IdComponent GetNumberOfIndices(const ConnectivityType& connectivity,
                                                             const IndexType& pointIndex)
  {
    return connectivity.GetNumberOfCellsOnPoint(pointIndex);
  }

  template <typename IndexType>
  VTKM_EXEC_CONT static IndicesType GetIndices(const ConnectivityType& connectivity,
                                               const IndexType& pointIndex)
  {
    return connectivity.GetCellsOfPoint(pointIndex);
  }

  VTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalFromIndex(const ConnectivityType& connectivity,
                                                 vtkm::Id flatFromIndex)
  {
    return connectivity.FlatToLogicalCellIndex(flatFromIndex);
  }

  VTKM_EXEC_CONT
  static vtkm::Id LogicalToFlatFromIndex(const ConnectivityType& connectivity,
                                         const LogicalIndexType& logicalFromIndex)
  {
    return connectivity.LogicalToFlatCellIndex(logicalFromIndex);
  }

  VTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalToIndex(const ConnectivityType& connectivity,
                                               vtkm::Id flatToIndex)
  {
    return connectivity.FlatToLogicalPointIndex(flatToIndex);
  }

  VTKM_EXEC_CONT
  static vtkm::Id LogicalToFlatToIndex(const ConnectivityType& connectivity,
                                       const LogicalIndexType& logicalToIndex)
  {
    return connectivity.LogicalToFlatPointIndex(logicalToIndex);
  }
};
}
} // namespace vtkm::internal

#endif //vtk_m_internal_ConnectivityStructuredInternals_h
