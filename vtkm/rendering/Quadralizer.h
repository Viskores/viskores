//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Quadralizer_h
#define vtk_m_rendering_Quadralizer_h

#include <typeinfo>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>


#define QUAD_PER_CSS 6

namespace vtkm
{
namespace rendering
{

class Quadralizer
{
public:
  class CountQuads : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    CountQuads() {}
    typedef void ControlSignature(CellSetIn cellset, FieldOut);
    typedef void ExecutionSignature(CellShape, _2);

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagGeneric shapeType, vtkm::Id& quads) const
    {
      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
        quads = 1;
      else if (shapeType.Id == CELL_SHAPE_HEXAHEDRON)
        quads = 6;
      else if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
        quads = 3;
      else if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
        quads = 1;

      else
        quads = 0;
    }

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType), vtkm::Id& quads) const
    {
      quads = 6;
    }

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagQuad shapeType, vtkm::Id& quads) const
    {
      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
        quads = 1;
      else
        quads = 0;
    }
    VTKM_EXEC
    void operator()(vtkm::CellShapeTagWedge vtkmNotUsed(shapeType), vtkm::Id& quads) const
    {
      quads = 3;
    }
  }; //class CountQuads

  template <int DIM>
  class SegmentedStructured : public vtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(IncidentElementIndices, _2, _3);
    //typedef _1 InputDomain;
    VTKM_CONT
    SegmentedStructured() {}

#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    template <typename CellNodeVecType, typename OutIndicesPortal>
    VTKM_EXEC void cell2quad(vtkm::Id4 idx,
                             vtkm::Vec<Id, 5>& quad,
                             const vtkm::Id offset,
                             const CellNodeVecType& cellIndices,
                             OutIndicesPortal& outputIndices) const
    {

      quad[1] = cellIndices[vtkm::IdComponent(idx[0])];
      quad[2] = cellIndices[vtkm::IdComponent(idx[1])];
      quad[3] = cellIndices[vtkm::IdComponent(idx[2])];
      quad[4] = cellIndices[vtkm::IdComponent(idx[3])];
      outputIndices.Set(offset, quad);
    }

    template <typename CellNodeVecType, typename OutIndicesPortal>
    VTKM_EXEC void operator()(const CellNodeVecType& cellIndices,
                              const vtkm::Id& cellIndex,
                              OutIndicesPortal& outputIndices) const
    {
      if (DIM == 2)
      {
        // Do nothing mark says
      }
      else if (DIM == 3)
      {
        vtkm::Id offset = cellIndex * QUAD_PER_CSS;
        vtkm::Vec<vtkm::Id, 5> quad;
        quad[0] = cellIndex;
        vtkm::Id4 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5, idx[3] = 4;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        idx[3] = 5;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        idx[3] = 2;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        idx[3] = 3;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        idx[3] = 1;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        idx[3] = 7;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);
      }
    }
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class Quadralize : public vtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    VTKM_CONT
    Quadralize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void cell2quad(vtkm::Id& offset,
                             const VecType& cellIndices,
                             const vtkm::Id& cellId,
                             const vtkm::Id Id0,
                             const vtkm::Id Id1,
                             const vtkm::Id Id2,
                             const vtkm::Id Id3,
                             OutputPortal& outputIndices) const
    {
      vtkm::Vec<vtkm::Id, 5> quad;
      quad[0] = cellId;
      quad[1] = static_cast<vtkm::Id>(cellIndices[vtkm::IdComponent(Id0)]);
      quad[2] = static_cast<vtkm::Id>(cellIndices[vtkm::IdComponent(Id1)]);
      quad[3] = static_cast<vtkm::Id>(cellIndices[vtkm::IdComponent(Id2)]);
      quad[4] = static_cast<vtkm::Id>(cellIndices[vtkm::IdComponent(Id3)]);
      outputIndices.Set(offset++, quad);
    }

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagWedge vtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      vtkm::Id offset = pointOffset;

      cell2quad(offset, cellIndices, cellId, 3, 0, 2, 5, outputIndices);
      cell2quad(offset, cellIndices, cellId, 1, 4, 5, 2, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 3, 4, 1, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& offset,
                              vtkm::CellShapeTagQuad shapeType,
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
      {
        vtkm::Vec<vtkm::Id, 5> quad;
        quad[0] = cellId;
        quad[1] = static_cast<vtkm::Id>(cellIndices[0]);
        quad[2] = static_cast<vtkm::Id>(cellIndices[1]);
        quad[3] = static_cast<vtkm::Id>(cellIndices[2]);
        quad[4] = static_cast<vtkm::Id>(cellIndices[3]);
        outputIndices.Set(offset, quad);
      }
    }

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const

    {
      vtkm::Id offset = pointOffset;
      cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
      cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
      cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
      cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
    }

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagGeneric shapeType,
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {

      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
      {
        vtkm::Vec<vtkm::Id, 5> quad;
        quad[0] = cellId;
        quad[1] = cellIndices[0];
        quad[2] = cellIndices[1];
        quad[3] = cellIndices[2];
        quad[4] = cellIndices[3];
        outputIndices.Set(pointOffset, quad);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
      {
        vtkm::Id offset = pointOffset;
        cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
        cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
        cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
        cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
      {
        vtkm::Id offset = pointOffset;

        cell2quad(offset, cellIndices, cellId, 3, 0, 2, 5, outputIndices);
        cell2quad(offset, cellIndices, cellId, 1, 4, 5, 2, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 3, 4, 1, outputIndices);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
      {
        vtkm::Id offset = pointOffset;

        cell2quad(offset, cellIndices, cellId, 3, 2, 1, 0, outputIndices);
      }
    }

  }; //class Quadralize

public:
  VTKM_CONT
  Quadralizer() {}

  VTKM_CONT
  void Run(const vtkm::cont::DynamicCellSet& cellset,
           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>>& outputIndices,
           vtkm::Id& output)
  {
    if (cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      vtkm::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.Cast<vtkm::cont::CellSetStructured<3>>();
      const vtkm::Id numCells = cellSetStructured3D.GetNumberOfCells();

      vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * QUAD_PER_CSS);
      vtkm::worklet::DispatcherMapTopology<SegmentedStructured<3>> segInvoker;
      segInvoker.Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      output = numCells * QUAD_PER_CSS;
    }
    else
    {
      vtkm::cont::ArrayHandle<vtkm::Id> quadsPerCell;
      vtkm::worklet::DispatcherMapTopology<CountQuads> countInvoker;
      countInvoker.Invoke(cellset, quadsPerCell);

      vtkm::Id total = 0;
      total = vtkm::cont::Algorithm::Reduce(quadsPerCell, vtkm::Id(0));

      vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
      vtkm::cont::Algorithm::ScanExclusive(quadsPerCell, cellOffsets);
      outputIndices.Allocate(total);

      vtkm::worklet::DispatcherMapTopology<Quadralize> quadInvoker;
      quadInvoker.Invoke(cellset, cellOffsets, outputIndices);

      output = total;
    }
  }
};
}
}
#endif
