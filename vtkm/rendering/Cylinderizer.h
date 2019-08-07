//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Cylinderizer_h
#define vtk_m_rendering_Cylinderizer_h

#include <typeinfo>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#define SEG_PER_TRI 3
//CSS is CellSetStructured
#define TRI_PER_CSS 12

namespace vtkm
{
namespace rendering
{

class Cylinderizer
{
public:
  class CountSegments : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    CountSegments() {}
    typedef void ControlSignature(CellSetIn cellset, FieldOut);
    typedef void ExecutionSignature(CellShape, _2);

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagGeneric shapeType, vtkm::Id& segments) const
    {
      if (shapeType.Id == vtkm::CELL_SHAPE_LINE)
        segments = 1;
      else if (shapeType.Id == vtkm::CELL_SHAPE_TRIANGLE)
        segments = 3;
      else if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
        segments = 4;
      else if (shapeType.Id == vtkm::CELL_SHAPE_TETRA)
        segments = 12;
      else if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
        segments = 24;
      else if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
        segments = 18;
      else if (shapeType.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
        segments = 36;
      else
        segments = 0;
    }

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType), vtkm::Id& segments) const
    {
      segments = 36;
    }

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagQuad vtkmNotUsed(shapeType), vtkm::Id& segments) const
    {
      segments = 4;
    }
    VTKM_EXEC
    void operator()(vtkm::CellShapeTagWedge vtkmNotUsed(shapeType), vtkm::Id& segments) const
    {
      segments = 24;
    }
  }; //class CountSegments

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
    VTKM_EXEC void cell2seg(vtkm::Id3 idx,
                            vtkm::Vec<Id, 3>& segment,
                            const vtkm::Id offset,
                            const CellNodeVecType& cellIndices,
                            OutIndicesPortal& outputIndices) const
    {

      segment[1] = cellIndices[vtkm::IdComponent(idx[0])];
      segment[2] = cellIndices[vtkm::IdComponent(idx[1])];
      outputIndices.Set(offset, segment);

      segment[1] = cellIndices[vtkm::IdComponent(idx[1])];
      segment[2] = cellIndices[vtkm::IdComponent(idx[2])];
      outputIndices.Set(offset + 1, segment);

      segment[1] = cellIndices[vtkm::IdComponent(idx[2])];
      segment[2] = cellIndices[vtkm::IdComponent(idx[0])];
      outputIndices.Set(offset + 2, segment);
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
        vtkm::Id offset = cellIndex * TRI_PER_CSS * SEG_PER_TRI;
        vtkm::Id3 segment;
        segment[0] = cellIndex;
        vtkm::Id3 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 5;
        idx[2] = 4;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 6;
        idx[2] = 5;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 6;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 7;
        idx[2] = 3;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 2;
        idx[2] = 1;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 6;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
      }
    }
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class Cylinderize : public vtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    VTKM_CONT
    Cylinderize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void tri2seg(vtkm::Id& offset,
                           const VecType& cellIndices,
                           const vtkm::Id& cellId,
                           const vtkm::Id Id0,
                           const vtkm::Id Id1,
                           const vtkm::Id Id2,
                           OutputPortal& outputIndices) const
    {
      vtkm::Id3 segment;
      segment[0] = cellId;
      segment[1] = vtkm::Id(cellIndices[vtkm::IdComponent(Id0)]);
      segment[2] = vtkm::Id(cellIndices[vtkm::IdComponent(Id1)]);
      outputIndices.Set(offset++, segment);

      segment[1] = vtkm::Id(cellIndices[vtkm::IdComponent(Id1)]);
      segment[2] = vtkm::Id(cellIndices[vtkm::IdComponent(Id2)]);
      outputIndices.Set(offset++, segment);

      segment[1] = vtkm::Id(cellIndices[vtkm::IdComponent(Id2)]);
      segment[2] = vtkm::Id(cellIndices[vtkm::IdComponent(Id0)]);
      outputIndices.Set(offset++, segment);
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
        vtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(offset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(offset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(offset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(offset + 3, segment);
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
      tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagWedge vtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const

    {
      vtkm::Id offset = pointOffset;
      tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagGeneric shapeType,
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {

      if (shapeType.Id == vtkm::CELL_SHAPE_LINE)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;

        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_TRIANGLE)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 2, segment);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(pointOffset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 3, segment);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_TETRA)
      {
        vtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 3, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
      {
        vtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
      {
        vtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
      {
        vtkm::Id offset = pointOffset;

        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 2, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 1, 0, outputIndices);
      }
    }

  }; //class cylinderize

public:
  VTKM_CONT
  Cylinderizer() {}

  VTKM_CONT
  void Run(const vtkm::cont::DynamicCellSet& cellset,
           vtkm::cont::ArrayHandle<vtkm::Id3>& outputIndices,
           vtkm::Id& output)
  {
    if (cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      vtkm::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.Cast<vtkm::cont::CellSetStructured<3>>();
      const vtkm::Id numCells = cellSetStructured3D.GetNumberOfCells();

      vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * TRI_PER_CSS * SEG_PER_TRI);

      vtkm::worklet::DispatcherMapTopology<SegmentedStructured<3>> segInvoker;
      segInvoker.Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      output = numCells * TRI_PER_CSS * SEG_PER_TRI;
    }
    else
    {
      vtkm::cont::ArrayHandle<vtkm::Id> segmentsPerCell;
      vtkm::worklet::DispatcherMapTopology<CountSegments> countInvoker;
      countInvoker.Invoke(cellset, segmentsPerCell);

      vtkm::Id total = 0;
      total = vtkm::cont::Algorithm::Reduce(segmentsPerCell, vtkm::Id(0));

      vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
      vtkm::cont::Algorithm::ScanExclusive(segmentsPerCell, cellOffsets);
      outputIndices.Allocate(total);

      vtkm::worklet::DispatcherMapTopology<Cylinderize> cylInvoker;
      cylInvoker.Invoke(cellset, cellOffsets, outputIndices);

      output = total;
    }
  }
};
}
}
#endif
