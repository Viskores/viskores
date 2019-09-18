//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/raytracing/QuadExtractor.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/rendering/Quadralizer.h>
#include <vtkm/rendering/raytracing/Worklets.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

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
    else
      quads = 0;
  }

  VTKM_EXEC
  void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType), vtkm::Id& quads) const
  {
    quads = 6;
  }

  VTKM_EXEC
  void operator()(vtkm::CellShapeTagQuad vtkmNotUsed(shapeType), vtkm::Id& points) const
  {
    points = 1;
  }
  VTKM_EXEC
  void operator()(vtkm::CellShapeTagWedge vtkmNotUsed(shapeType), vtkm::Id& points) const
  {
    points = 0;
  }

}; // ClassCountquads

class Pointify : public vtkm::worklet::WorkletVisitCellsWithPoints
{

public:
  VTKM_CONT
  Pointify() {}
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
  VTKM_EXEC void operator()(const vtkm::Id& vtkmNotUsed(pointOffset),
                            vtkm::CellShapeTagQuad vtkmNotUsed(shapeType),
                            const VecType& vtkmNotUsed(cellIndices),
                            const vtkm::Id& vtkmNotUsed(cellId),
                            OutputPortal& vtkmNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  VTKM_EXEC void operator()(const vtkm::Id& vtkmNotUsed(pointOffset),
                            vtkm::CellShapeTagWedge vtkmNotUsed(shapeType),
                            const VecType& vtkmNotUsed(cellIndices),
                            const vtkm::Id& vtkmNotUsed(cellId),
                            OutputPortal& vtkmNotUsed(outputIndices)) const
  {
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
  }
}; //class pointify

class Iterator : public vtkm::worklet::WorkletMapField
{

public:
  VTKM_CONT
  Iterator() {}
  typedef void ControlSignature(FieldOut);
  typedef void ExecutionSignature(_1, WorkIndex);
  VTKM_EXEC
  void operator()(vtkm::Id2& index, const vtkm::Id2& idx) const { index = idx; }
}; //class Iterator

class FieldRadius : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Float32 MinRadius;
  vtkm::Float32 RadiusDelta;
  vtkm::Float32 MinValue;
  vtkm::Float32 InverseDelta;

public:
  VTKM_CONT
  FieldRadius(const vtkm::Float32 minRadius,
              const vtkm::Float32 maxRadius,
              const vtkm::Range scalarRange)
    : MinRadius(minRadius)
    , RadiusDelta(maxRadius - minRadius)
    , MinValue(static_cast<vtkm::Float32>(scalarRange.Min))
  {
    vtkm::Float32 delta = static_cast<vtkm::Float32>(scalarRange.Max - scalarRange.Min);
    if (delta != 0.f)
      InverseDelta = 1.f / (delta);
    else
      InverseDelta = 0.f; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename ScalarPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            vtkm::Float32& radius,
                            const ScalarPortalType& scalars) const
  {
    vtkm::Float32 scalar = static_cast<vtkm::Float32>(scalars.Get(pointId));
    vtkm::Float32 t = (scalar - MinValue) * InverseDelta;
    radius = MinRadius + t * RadiusDelta;
  }

}; //class FieldRadius

} //namespace detail

void QuadExtractor::ExtractCells(const vtkm::cont::DynamicCellSet& cells)
{
  vtkm::Id numOfQuads;
  vtkm::rendering::Quadralizer quadrizer;
  quadrizer.Run(cells, this->QuadIds, numOfQuads);

  //this->SetPointIdsFromCells(cells);
}


void QuadExtractor::SetQuadIdsFromCells(const vtkm::cont::DynamicCellSet& cells)
{
  vtkm::Id numCells = cells.GetNumberOfCells();
  if (numCells == 0)
  {
    return;
  }
  //
  // look for points in the cell set
  //
  if (cells.IsSameType(vtkm::cont::CellSetExplicit<>()))
  {
    vtkm::cont::ArrayHandle<vtkm::Id> points;
    vtkm::worklet::DispatcherMapTopology<detail::CountQuads>(detail::CountQuads())
      .Invoke(cells, points);

    vtkm::Id total = 0;
    total = vtkm::cont::Algorithm::Reduce(points, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
    vtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    QuadIds.Allocate(total);

    vtkm::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cells, cellOffsets, this->QuadIds);
  }
}


vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>> QuadExtractor::GetQuadIds()
{
  return this->QuadIds;
}


vtkm::Id QuadExtractor::GetNumberOfQuads() const
{
  return this->QuadIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
