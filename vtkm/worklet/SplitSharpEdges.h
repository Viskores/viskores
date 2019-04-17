//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_SplitSharpEdges_h
#define vtk_m_worklet_SplitSharpEdges_h

#include <map>

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/Bitset.h>
#include <vtkm/CellTraits.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/exec/CellEdge.h>

namespace vtkm
{
namespace worklet
{

namespace internal
{
// Given a cell and a point on the cell, find the two edges that are
// associated with this point in canonical index
template <typename PointFromCellSetType>
VTKM_EXEC void FindRelatedEdges(const vtkm::Id& pointIndex,
                                const vtkm::Id& cellIndexG,
                                const PointFromCellSetType& pFromCellSet,
                                vtkm::Id2& edge0G,
                                vtkm::Id2& edge1G,
                                const vtkm::exec::FunctorBase& worklet)
{
  typename PointFromCellSetType::CellShapeTag cellShape = pFromCellSet.GetCellShape(cellIndexG);
  typename PointFromCellSetType::IndicesType cellConnections = pFromCellSet.GetIndices(cellIndexG);
  vtkm::IdComponent numPointsInCell = pFromCellSet.GetNumberOfIndices(cellIndexG);
  vtkm::IdComponent numEdges =
    vtkm::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, worklet);
  vtkm::IdComponent edgeIndex = -1;
  // Find the two edges with the pointIndex
  while (true)
  {
    ++edgeIndex;
    if (edgeIndex >= numEdges)
    {
      worklet.RaiseError("Bad cell. Could not find two incident edges.");
      return;
    }
    vtkm::Id2 canonicalEdgeId(cellConnections[vtkm::exec::CellEdgeLocalIndex(
                                numPointsInCell, 0, edgeIndex, cellShape, worklet)],
                              cellConnections[vtkm::exec::CellEdgeLocalIndex(
                                numPointsInCell, 1, edgeIndex, cellShape, worklet)]);
    if (canonicalEdgeId[0] == pointIndex || canonicalEdgeId[1] == pointIndex)
    { // Assign value to edge0 first
      if ((edge0G[0] == -1) && (edge0G[1] == -1))
      {
        edge0G = canonicalEdgeId;
      }
      else
      {
        edge1G = canonicalEdgeId;
        break;
      }
    }
  }
}

// TODO: We should replace this expensive lookup with a WholeCellSetIn<Edge, Cell> map.
// Given an edge on a cell, it would find the neighboring
// cell of this edge in local index. If it's a non manifold edge, -1 would be returned.
template <typename PointFromCellSetType, typename IncidentCellVecType>
VTKM_EXEC int FindNeighborCellInLocalIndex(const vtkm::Id2& eOI,
                                           const PointFromCellSetType& pFromCellSet,
                                           const IncidentCellVecType& incidentCells,
                                           const vtkm::Id currentCellLocalIndex,
                                           const vtkm::exec::FunctorBase& worklet)
{
  int neighboringCellIndex = -1;
  vtkm::IdComponent numberOfIncidentCells = incidentCells.GetNumberOfComponents();
  size_t neighboringCellsCount = 0;
  for (vtkm::IdComponent incidentCellIndex = 0; incidentCellIndex < numberOfIncidentCells;
       incidentCellIndex++)
  {
    if (currentCellLocalIndex == incidentCellIndex)
    {
      continue; // No need to check the current interested cell
    }
    vtkm::Id cellIndexG = incidentCells[incidentCellIndex]; // Global cell index
    typename PointFromCellSetType::CellShapeTag cellShape = pFromCellSet.GetCellShape(cellIndexG);
    typename PointFromCellSetType::IndicesType cellConnections =
      pFromCellSet.GetIndices(cellIndexG);
    vtkm::IdComponent numPointsInCell = pFromCellSet.GetNumberOfIndices(cellIndexG);
    vtkm::IdComponent numEdges =
      vtkm::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, worklet);
    vtkm::IdComponent edgeIndex = -1;
    // Check if this cell has edge of interest
    while (true)
    {
      ++edgeIndex;
      if (edgeIndex >= numEdges)
      {
        break;
      }
      vtkm::Id2 canonicalEdgeId(cellConnections[vtkm::exec::CellEdgeLocalIndex(
                                  numPointsInCell, 0, edgeIndex, cellShape, worklet)],
                                cellConnections[vtkm::exec::CellEdgeLocalIndex(
                                  numPointsInCell, 1, edgeIndex, cellShape, worklet)]);
      if ((canonicalEdgeId[0] == eOI[0] && canonicalEdgeId[1] == eOI[1]) ||
          (canonicalEdgeId[0] == eOI[1] && canonicalEdgeId[1] == eOI[0]))
      {
        neighboringCellIndex = incidentCellIndex;
        neighboringCellsCount++;
        break;
      }
    }
  }
  return neighboringCellIndex;
}
} // internal namespace

// Split sharp manifold edges where the feature angle between the
// adjacent surfaces are larger than the threshold value
class SplitSharpEdges
{
public:
  // This worklet would calculate the needed space for splitting sharp edges.
  // For each point, it would have two values as numberOfNewPoint(how many
  // times this point needs to be duplicated) and numberOfCellsNeedsUpdate
  // (how many neighboring cells need to update connectivity).
  // For example, Given a unit cube and feature angle
  // as 89 degree, each point would be duplicated twice and there are two cells
  // need connectivity update. There is no guarantee on which cell would get which
  // new point.
  class ClassifyPoint : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    ClassifyPoint(vtkm::FloatDefault cosfeatureAngle)
      : CosFeatureAngle(cosfeatureAngle)
    {
    }
    using ControlSignature = void(CellSetIn intputCells,
                                  WholeCellSetIn<Point, Cell>, // Query points from cell
                                  FieldInCell faceNormals,
                                  FieldOutPoint newPointNum,
                                  FieldOutPoint cellNum);
    using ExecutionSignature = void(CellIndices incidentCells,
                                    InputIndex pointIndex,
                                    _2 pFromCellSet,
                                    _3 faceNormals,
                                    _4 newPointNum,
                                    _5 cellNum);
    using InputDomain = _1;

    template <typename IncidentCellVecType,
              typename PointFromCellSetType,
              typename FaceNormalVecType>
    VTKM_EXEC void operator()(const IncidentCellVecType& incidentCells,
                              vtkm::Id pointIndex,
                              const PointFromCellSetType& pFromCellSet,
                              const FaceNormalVecType& faceNormals,
                              vtkm::Id& newPointNum,
                              vtkm::Id& cellNum) const
    {
      vtkm::IdComponent numberOfIncidentCells = incidentCells.GetNumberOfComponents();

      VTKM_ASSERT(numberOfIncidentCells < 64);

      if (numberOfIncidentCells <= 1)
      {
        return; // Not enought cells to compare
      }

      // Initialize a global cell mask to avoid confusion. globalCellIndex->status
      // 0 means not visited yet 1 means visited.
      vtkm::Bitset<vtkm::UInt64> visitedCells;
      vtkm::Id visitedCellsRegionIndex[64] = { 0 };
      // Reallocate memory for visitedCellsGroup if needed

      int regionIndex = 0;
      // Loop through each cell
      for (vtkm::IdComponent incidentCellIndex = 0; incidentCellIndex < numberOfIncidentCells;
           incidentCellIndex++)
      {
        vtkm::Id cellIndexG = incidentCells[incidentCellIndex]; // cell index in global order
        // If not visited
        if (!visitedCells.test(incidentCellIndex))
        {
          // Mark the cell and track the region
          visitedCells.set(incidentCellIndex);
          visitedCellsRegionIndex[incidentCellIndex] = regionIndex;

          // Find two edges containing the current point in canonial index
          vtkm::Id2 edge0G(-1, -1), edge1G(-1, -1);
          internal::FindRelatedEdges(pointIndex, cellIndexG, pFromCellSet, edge0G, edge1G, *this);
          // Grow the area along each edge
          for (size_t i = 0; i < 2; i++)
          { // Reset these two values for each grow operation
            vtkm::Id2 currentEdgeG = i == 0 ? edge0G : edge1G;
            vtkm::IdComponent currentTestingCellIndex = incidentCellIndex;
            while (currentTestingCellIndex >= 0)
            {
              // Find the neighbor cell of the current cell edge in local index
              int neighboringCellIndexQuery = internal::FindNeighborCellInLocalIndex(
                currentEdgeG, pFromCellSet, incidentCells, currentTestingCellIndex, *this);
              // The edge should be manifold and the neighboring cell should
              // have not been visited
              if (neighboringCellIndexQuery != -1 && !visitedCells.test(neighboringCellIndexQuery))
              {
                vtkm::IdComponent neighborCellIndex =
                  static_cast<vtkm::IdComponent>(neighboringCellIndexQuery);
                auto thisNormal = faceNormals[currentTestingCellIndex];
                //neighborNormal
                auto neighborNormal = faceNormals[neighborCellIndex];
                // Try to grow the area
                if (vtkm::dot(thisNormal, neighborNormal) > this->CosFeatureAngle)
                { // No need to split.
                  visitedCells.set(neighborCellIndex);
                  visitedCellsRegionIndex[neighborCellIndex] = regionIndex;

                  // Move to examine next cell
                  currentTestingCellIndex = neighborCellIndex;
                  vtkm::Id2 neighborCellEdge0G(-1, -1), neighborCellEdge1G(-1, -1);
                  internal::FindRelatedEdges(pointIndex,
                                             incidentCells[currentTestingCellIndex],
                                             pFromCellSet,
                                             neighborCellEdge0G,
                                             neighborCellEdge1G,
                                             *this);
                  // Update currentEdgeG
                  if ((currentEdgeG == neighborCellEdge0G) ||
                      currentEdgeG == vtkm::Id2(neighborCellEdge0G[1], neighborCellEdge0G[0]))
                  {
                    currentEdgeG = neighborCellEdge1G;
                  }
                  else
                  {
                    currentEdgeG = neighborCellEdge0G;
                  }
                }
                else
                {
                  currentTestingCellIndex = -1;
                }
              }
              else
              {
                currentTestingCellIndex =
                  -1; // Either seperated by previous visit, boundary or non-manifold
              }
              // cells is smaller than the thresold and the nighboring cell has not been visited
            }
          }
          regionIndex++;
        }
      }

      // For each new region you need a new point
      newPointNum = regionIndex - 1;
      vtkm::Id numberOfCellsNeedUpdate = 0;
      for (vtkm::IdComponent i = 0; i < numberOfIncidentCells; i++)
      {
        if (visitedCellsRegionIndex[i] > 0)
        {
          numberOfCellsNeedUpdate++;
        }
      }
      cellNum = numberOfCellsNeedUpdate;
    }

  private:
    vtkm::FloatDefault CosFeatureAngle; // Cos value of the feature angle
  };

  // This worklet split the sharp edges and populate the
  // cellTopologyUpdateTuples as (cellGlobalId, oldPointId, newPointId).
  class SplitSharpEdge : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    SplitSharpEdge(vtkm::FloatDefault cosfeatureAngle, vtkm::Id numberOfOldPoints)
      : CosFeatureAngle(cosfeatureAngle)
      , NumberOfOldPoints(numberOfOldPoints)
    {
    }
    using ControlSignature = void(CellSetIn intputCells,
                                  WholeCellSetIn<Point, Cell>, // Query points from cell
                                  FieldInCell faceNormals,
                                  FieldInPoint newPointStartingIndex,
                                  FieldInPoint pointCellsStartingIndex,
                                  WholeArrayOut cellTopologyUpdateTuples);
    using ExecutionSignature = void(CellIndices incidentCells,
                                    InputIndex pointIndex,
                                    _2 pFromCellSet,
                                    _3 faceNormals,
                                    _4 newPointStartingIndex,
                                    _5 pointCellsStartingIndex,
                                    _6 cellTopologyUpdateTuples);
    using InputDomain = _1;

    template <typename IncidentCellVecType,
              typename PointFromCellSetType,
              typename FaceNormalVecType,
              typename CellTopologyUpdateTuples>
    VTKM_EXEC void operator()(const IncidentCellVecType& incidentCells,
                              vtkm::Id pointIndex,
                              const PointFromCellSetType& pFromCellSet,
                              const FaceNormalVecType& faceNormals,
                              const vtkm::Id& newPointStartingIndex,
                              const vtkm::Id& pointCellsStartingIndex,
                              CellTopologyUpdateTuples& cellTopologyUpdateTuples) const
    {
      vtkm::IdComponent numberOfIncidentCells = incidentCells.GetNumberOfComponents();
      if (numberOfIncidentCells <= 1)
      {
        return; // Not enought cells to compare
      }
      // Initialize a global cell mask to avoid confusion. globalCellIndex->status
      // 0 means not visited yet 1 means visited.
      vtkm::Bitset<vtkm::UInt64> visitedCells;
      vtkm::Id visitedCellsRegionIndex[64] = { 0 };
      // Reallocate memory for visitedCellsGroup if needed

      int regionIndex = 0;
      // Loop through each cell
      for (vtkm::IdComponent incidentCellIndex = 0; incidentCellIndex < numberOfIncidentCells;
           incidentCellIndex++)
      {
        vtkm::Id cellIndexG = incidentCells[incidentCellIndex]; // cell index in global order
        // If not visited
        if (!visitedCells.test(incidentCellIndex))
        {
          // Mark the cell and track the region
          visitedCells.set(incidentCellIndex);
          visitedCellsRegionIndex[incidentCellIndex] = regionIndex;

          // Find two edges containing the current point in canonial index
          vtkm::Id2 edge0G(-1, -1), edge1G(-1, -1);
          internal::FindRelatedEdges(pointIndex, cellIndexG, pFromCellSet, edge0G, edge1G, *this);
          // Grow the area along each edge
          for (size_t i = 0; i < 2; i++)
          { // Reset these two values for each grow operation
            vtkm::Id2 currentEdgeG = i == 0 ? edge0G : edge1G;
            vtkm::IdComponent currentTestingCellIndex = incidentCellIndex;
            while (currentTestingCellIndex >= 0)
            {
              // Find the neighbor of the current cell edge in local index
              int neighboringCellIndexQuery = internal::FindNeighborCellInLocalIndex(
                currentEdgeG, pFromCellSet, incidentCells, currentTestingCellIndex, *this);
              // The edge should be manifold and the neighboring cell should
              // have not been visited
              if (neighboringCellIndexQuery != -1 && !visitedCells.test(neighboringCellIndexQuery))
              {
                vtkm::IdComponent neighborCellIndex =
                  static_cast<vtkm::IdComponent>(neighboringCellIndexQuery);
                // Try to grow the area if the feature angle between current neighbor
                auto thisNormal = faceNormals[currentTestingCellIndex];
                //neighborNormal
                auto neighborNormal = faceNormals[neighborCellIndex];
                if (vtkm::dot(thisNormal, neighborNormal) > this->CosFeatureAngle)
                { // No need to split.
                  visitedCells.set(neighborCellIndex);
                  // Move to examine next cell
                  currentTestingCellIndex = neighborCellIndex;
                  vtkm::Id2 neighborCellEdge0G(-1, -1), neighborCellEdge1G(-1, -1);
                  internal::FindRelatedEdges(pointIndex,
                                             incidentCells[currentTestingCellIndex],
                                             pFromCellSet,
                                             neighborCellEdge0G,
                                             neighborCellEdge1G,
                                             *this);
                  // Update currentEdgeG
                  if ((currentEdgeG == neighborCellEdge0G) ||
                      currentEdgeG == vtkm::Id2(neighborCellEdge0G[1], neighborCellEdge0G[0]))
                  {
                    currentEdgeG = neighborCellEdge1G;
                  }
                  else
                  {
                    currentEdgeG = neighborCellEdge0G;
                  }
                }
                else
                {
                  currentTestingCellIndex = -1;
                }
              }
              else
              {
                currentTestingCellIndex =
                  -1; // Either seperated by previous visit, boundary or non-manifold
              }
              // cells is smaller than the thresold and the nighboring cell has not been visited
            }
          }
          regionIndex++;
        }
      }

      // For each new region you need a new point
      // Initialize the offset in the global cellTopologyUpdateTuples;
      vtkm::Id cellTopologyUpdateTuplesIndex = pointCellsStartingIndex;

      for (vtkm::Id i = 0; i < numberOfIncidentCells; i++)
      {
        if (visitedCellsRegionIndex[i])
        { // New region generated. Need to update the topology
          vtkm::Id replacementPointId =
            NumberOfOldPoints + newPointStartingIndex + visitedCellsRegionIndex[i] - 1;
          vtkm::Id globalCellId = incidentCells[static_cast<vtkm::IdComponent>(i)];
          // (cellGlobalIndex, oldPointId, replacementPointId)
          vtkm::Vec<vtkm::Id, 3> tuple =
            vtkm::make_Vec(globalCellId, pointIndex, replacementPointId);
          cellTopologyUpdateTuples.Set(cellTopologyUpdateTuplesIndex, tuple);
          cellTopologyUpdateTuplesIndex++;
        }
      }
    }

  private:
    vtkm::FloatDefault CosFeatureAngle; // Cos value of the feature angle
    vtkm::Id NumberOfOldPoints;
  };

  template <typename CellSetType,
            typename FaceNormalsType,
            typename CoordsComType,
            typename CoordsInStorageType,
            typename CoordsOutStorageType,
            typename NewCellSetType>
  void Run(
    const CellSetType& oldCellset,
    const vtkm::FloatDefault featureAngle,
    const FaceNormalsType& faceNormals,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsComType, 3>, CoordsInStorageType>& oldCoords,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordsComType, 3>, CoordsOutStorageType>& newCoords,
    NewCellSetType& newCellset)
  {
    vtkm::FloatDefault featureAngleR =
      featureAngle / static_cast<vtkm::FloatDefault>(180.0) * vtkm::Pi<vtkm::FloatDefault>();

    ClassifyPoint classifyPoint(vtkm::Cos(featureAngleR));
    vtkm::worklet::DispatcherMapTopology<ClassifyPoint> cpDispatcher(classifyPoint);

    // Array of newPointNums and cellNeedUpdateNums
    vtkm::cont::ArrayHandle<vtkm::Id> newPointNums, cellNeedUpdateNums;
    cpDispatcher.Invoke(oldCellset, oldCellset, faceNormals, newPointNums, cellNeedUpdateNums);

    vtkm::Id totalNewPointsNum =
      vtkm::cont::Algorithm::Reduce(newPointNums, vtkm::Id(0), vtkm::Add());
    // Allocate the size for the updateCellTopologyArray
    vtkm::Id cellsNeedUpdateNum =
      vtkm::cont::Algorithm::Reduce(cellNeedUpdateNums, vtkm::Id(0), vtkm::Add());

    vtkm::cont::ArrayHandle<vtkm::Id3> cellTopologyUpdateTuples;
    cellTopologyUpdateTuples.Allocate(cellsNeedUpdateNum);

    vtkm::cont::ArrayHandle<vtkm::Id> newpointStartingIndexs, pointCellsStartingIndexs;
    vtkm::cont::Algorithm::ScanExclusive(newPointNums, newpointStartingIndexs);
    vtkm::cont::Algorithm::ScanExclusive(cellNeedUpdateNums, pointCellsStartingIndexs);

    SplitSharpEdge splitSharpEdge(vtkm::Cos(featureAngleR), oldCoords.GetNumberOfValues());

    vtkm::worklet::DispatcherMapTopology<SplitSharpEdge> sseDispatcher(splitSharpEdge);
    sseDispatcher.Invoke(oldCellset,
                         oldCellset,
                         faceNormals,
                         newpointStartingIndexs,
                         pointCellsStartingIndexs,
                         cellTopologyUpdateTuples);
    auto ctutPortal = cellTopologyUpdateTuples.GetPortalConstControl();

    // Create the new point coordinate system and update NewPointsIdArray to
    // process point field
    this->NewPointsIdArray.Allocate(oldCoords.GetNumberOfValues() + totalNewPointsNum);
    auto newPointsIdArrayPortal = this->NewPointsIdArray.GetPortalControl();
    for (vtkm::Id i = 0; i < newPointNums.GetNumberOfValues(); i++)
    {
      newPointsIdArrayPortal.Set(i, i);
    }

    newCoords.Allocate(oldCoords.GetNumberOfValues() + totalNewPointsNum);
    vtkm::cont::Algorithm::CopySubRange(oldCoords, 0, oldCoords.GetNumberOfValues(), newCoords);
    vtkm::Id newCoordsIndex = oldCoords.GetNumberOfValues();
    auto oldCoordsPortal = oldCoords.GetPortalConstControl();
    auto newCoordsPortal = newCoords.GetPortalControl();
    auto newPointNumsPortal = newPointNums.GetPortalControl();

    for (vtkm::Id i = 0; i < newPointNums.GetNumberOfValues(); i++)
    { // Find out for each new point, how many times it should be added
      for (vtkm::Id j = 0; j < newPointNumsPortal.Get(i); j++)
      {
        newPointsIdArrayPortal.Set(newCoordsIndex, i);
        newCoordsPortal.Set(newCoordsIndex++, oldCoordsPortal.Get(i));
      }
    }

    // Create the new cellset
    CellDeepCopy::Run(oldCellset, newCellset);
    // FIXME: Since the non const get array function is not in CellSetExplict.h,
    // here I just get a non-const copy of the array handle.
    auto connectivityArrayHandle = newCellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                                   vtkm::TopologyElementTagCell());
    auto connectivityArrayHandleP = connectivityArrayHandle.GetPortalControl();
    auto offsetArrayHandle = newCellset.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(),
                                                            vtkm::TopologyElementTagCell());
    auto offsetArrayHandleP = offsetArrayHandle.GetPortalControl();
    for (vtkm::Id i = 0; i < cellTopologyUpdateTuples.GetNumberOfValues(); i++)
    {
      vtkm::Id cellId(ctutPortal.Get(i)[0]), oldPointId(ctutPortal.Get(i)[1]),
        newPointId(ctutPortal.Get(i)[2]);
      vtkm::Id bound = (cellId + 1 == offsetArrayHandle.GetNumberOfValues())
        ? connectivityArrayHandle.GetNumberOfValues()
        : offsetArrayHandleP.Get(cellId + 1);
      vtkm::Id k = 0;
      for (vtkm::Id j = offsetArrayHandleP.Get(cellId); j < bound; j++, k++)
      {
        if (connectivityArrayHandleP.Get(j) == oldPointId)
        {
          connectivityArrayHandleP.Set(j, newPointId);
        }
      }
    }
  }

  template <typename ValueType, typename StorageTag>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageTag> in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->NewPointsIdArray, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> NewPointsIdArray;
};
}
} // vtkm::worklet

#endif // vtk_m_worklet_SplitSharpEdges_h
