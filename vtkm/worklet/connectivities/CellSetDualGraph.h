//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_connectivity_CellSetDualGraph_h
#define vtk_m_worklet_connectivity_CellSetDualGraph_h

#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/exec/CellEdge.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
struct EdgeCount : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell numEdgesInCell);

  using ExecutionSignature = _2(CellShape, PointCount);

  using InputDomain = _1;

  template <typename CellShapeTag>
  VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag cellShape, vtkm::IdComponent pointCount) const
  {
    return vtkm::exec::CellEdgeNumberOfEdges(pointCount, cellShape, *this);
  }
};

struct EdgeExtract : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell cellIndices, FieldOutCell edgeIndices);

  using ExecutionSignature = void(CellShape, InputIndex, PointIndices, VisitIndex, _2, _3);

  using InputDomain = _1;

  using ScatterType = vtkm::worklet::ScatterCounting;

  template <typename CellShapeTag,
            typename CellIndexType,
            typename PointIndexVecType,
            typename EdgeIndexVecType>
  VTKM_EXEC void operator()(CellShapeTag cellShape,
                            CellIndexType cellIndex,
                            const PointIndexVecType& pointIndices,
                            vtkm::IdComponent visitIndex,
                            CellIndexType& cellIndexOut,
                            EdgeIndexVecType& edgeIndices) const
  {
    cellIndexOut = cellIndex;
    edgeIndices = vtkm::exec::CellEdgeCanonicalId(
      pointIndices.GetNumberOfComponents(), visitIndex, cellShape, pointIndices, *this);
  }
};

struct CellToCellConnectivity : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn index,
                                WholeArrayIn cells,
                                WholeArrayOut from,
                                WholeArrayOut to);

  using ExecutionSignature = void(_1, InputIndex, _2, _3, _4);

  using InputDomain = _1;

  template <typename ConnectivityPortalType, typename CellIdPortalType>
  VTKM_EXEC void operator()(vtkm::Id offset,
                            vtkm::Id index,
                            const CellIdPortalType& cells,
                            ConnectivityPortalType& from,
                            ConnectivityPortalType& to) const
  {
    from.Set(index * 2, cells.Get(offset));
    to.Set(index * 2, cells.Get(offset + 1));
    from.Set(index * 2 + 1, cells.Get(offset + 1));
    to.Set(index * 2 + 1, cells.Get(offset));
  }
};
} // vtkm::worklet::connectivity::detail

class CellSetDualGraph
{
public:
  using Algorithm = vtkm::cont::Algorithm;

  struct degree2
  {
    VTKM_EXEC
    bool operator()(vtkm::Id degree) const { return degree >= 2; }
  };

  template <typename CellSet>
  void EdgeToCellConnectivity(const CellSet& cellSet,
                              vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
                              vtkm::cont::ArrayHandle<vtkm::Id2>& cellEdges) const
  {
    // Get number of edges for each cell and use it as scatter count.
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numEdgesPerCell;
    vtkm::worklet::DispatcherMapTopology<detail::EdgeCount> edgesPerCellDisp;
    edgesPerCellDisp.Invoke(cellSet, numEdgesPerCell);

    // Get uncompress Cell to Edge mapping
    vtkm::worklet::ScatterCounting scatter{ numEdgesPerCell };
    vtkm::worklet::DispatcherMapTopology<detail::EdgeExtract> edgeExtractDisp{ scatter };
    edgeExtractDisp.Invoke(cellSet, cellIds, cellEdges);
  }

  template <typename CellSetType>
  void Run(const CellSetType& cellSet,
           vtkm::cont::ArrayHandle<vtkm::Id>& numIndicesArray,
           vtkm::cont::ArrayHandle<vtkm::Id>& indexOffsetArray,
           vtkm::cont::ArrayHandle<vtkm::Id>& connectivityArray) const
  {
    // calculate the uncompressed Edge to Cell connectivity from Point to Cell connectivity
    // in the CellSet
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    vtkm::cont::ArrayHandle<vtkm::Id2> cellEdges;
    EdgeToCellConnectivity(cellSet, cellIds, cellEdges);

    // sort cell ids by cell edges, this groups cells by cell edges
    Algorithm::SortByKey(cellEdges, cellIds);

    // count how many times an edge is shared by cells.
    vtkm::cont::ArrayHandle<vtkm::Id2> uniqueEdges;
    vtkm::cont::ArrayHandle<vtkm::Id> uniqueEdgeDegree;
    Algorithm::ReduceByKey(
      cellEdges,
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(1, cellEdges.GetNumberOfValues()),
      uniqueEdges,
      uniqueEdgeDegree,
      vtkm::Add());

    // Extract edges shared by two cells
    vtkm::cont::ArrayHandle<vtkm::Id2> sharedEdges;
    Algorithm::CopyIf(uniqueEdges, uniqueEdgeDegree, sharedEdges, degree2());

    // find shared edges within all the edges.
    vtkm::cont::ArrayHandle<vtkm::Id> lb;
    Algorithm::LowerBounds(cellEdges, sharedEdges, lb);

    // take each shared edge and the cells to create 2 edges of the dual graph
    vtkm::cont::ArrayHandle<vtkm::Id> connFrom;
    vtkm::cont::ArrayHandle<vtkm::Id> connTo;
    connFrom.Allocate(sharedEdges.GetNumberOfValues() * 2);
    connTo.Allocate(sharedEdges.GetNumberOfValues() * 2);
    vtkm::worklet::DispatcherMapField<detail::CellToCellConnectivity> c2cDisp;
    c2cDisp.Invoke(lb, cellIds, connFrom, connTo);

    // Turn dual graph into Compressed Sparse Row format
    Algorithm::SortByKey(connFrom, connTo);
    Algorithm::Copy(connTo, connectivityArray);

    vtkm::cont::ArrayHandle<vtkm::Id> dualGraphVertices;
    Algorithm::ReduceByKey(
      connFrom,
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(1, connFrom.GetNumberOfValues()),
      dualGraphVertices,
      numIndicesArray,
      vtkm::Add());
    Algorithm::ScanExclusive(numIndicesArray, indexOffsetArray);
  }
};
}
}
}

#endif //vtk_m_worklet_connectivity_CellSetDualGraph_h
