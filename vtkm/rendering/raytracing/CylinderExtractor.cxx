//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/raytracing/CylinderExtractor.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/rendering/Cylinderizer.h>
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
    else
      segments = 0;
  }

  VTKM_EXEC
  void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType), vtkm::Id& points) const
  {
    points = 0;
  }

  VTKM_EXEC
  void operator()(vtkm::CellShapeTagQuad vtkmNotUsed(shapeType), vtkm::Id& points) const
  {
    points = 0;
  }
  VTKM_EXEC
  void operator()(vtkm::CellShapeTagWedge vtkmNotUsed(shapeType), vtkm::Id& points) const
  {
    points = 0;
  }

}; // ClassCountSegments

class Pointify : public vtkm::worklet::WorkletVisitCellsWithPoints
{

public:
  VTKM_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

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
  VTKM_EXEC void operator()(const vtkm::Id& vtkmNotUsed(pointOffset),
                            vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType),
                            const VecType& vtkmNotUsed(cellIndices),
                            const vtkm::Id& vtkmNotUsed(cellId),
                            OutputPortal& vtkmNotUsed(outputIndices)) const

  {
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
    else if (shapeType.Id == vtkm::CELL_SHAPE_TRIANGLE)
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
    else if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
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
  VTKM_EXEC void operator()(const vtkm::Id3& cylId,
                            vtkm::Float32& radius,
                            const ScalarPortalType& scalars) const
  {
    vtkm::Float32 scalar = static_cast<vtkm::Float32>(scalars.Get(cylId[0]));
    vtkm::Float32 t = (scalar - MinValue) * InverseDelta;
    radius = MinRadius + t * RadiusDelta;
  }

}; //class FieldRadius

} //namespace detail


void CylinderExtractor::ExtractCells(const vtkm::cont::DynamicCellSet& cells,
                                     const vtkm::Float32 radius)
{
  vtkm::Id numOfSegments;
  vtkm::rendering::Cylinderizer geometrizer;
  geometrizer.Run(cells, this->CylIds, numOfSegments);

  this->SetUniformRadius(radius);
}

void CylinderExtractor::ExtractCells(const vtkm::cont::DynamicCellSet& cells,
                                     const vtkm::cont::Field& field,
                                     const vtkm::Float32 minRadius,
                                     const vtkm::Float32 maxRadius)
{
  vtkm::Id numOfSegments;
  vtkm::rendering::Cylinderizer geometrizer;
  geometrizer.Run(cells, this->CylIds, numOfSegments);

  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void CylinderExtractor::SetUniformRadius(const vtkm::Float32 radius)
{
  const vtkm::Id size = this->CylIds.GetNumberOfValues();
  Radii.Allocate(size);

  vtkm::cont::ArrayHandleConstant<vtkm::Float32> radiusHandle(radius, size);
  vtkm::cont::Algorithm::Copy(radiusHandle, Radii);
}

void CylinderExtractor::SetCylinderIdsFromCells(const vtkm::cont::DynamicCellSet& cells)
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
    vtkm::worklet::DispatcherMapTopology<detail::CountSegments>(detail::CountSegments())
      .Invoke(cells, points);

    vtkm::Id totalPoints = 0;
    totalPoints = vtkm::cont::Algorithm::Reduce(points, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
    vtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    CylIds.Allocate(totalPoints);

    vtkm::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cells, cellOffsets, this->CylIds);
  }
}

void CylinderExtractor::SetVaryingRadius(const vtkm::Float32 minRadius,
                                         const vtkm::Float32 maxRadius,
                                         const vtkm::cont::Field& field)
{
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw vtkm::cont::ErrorBadValue("Cylinder Extractor: scalar field must have one component");
  }

  vtkm::Range range = rangeArray.GetPortalConstControl().Get(0);

  Radii.Allocate(this->CylIds.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<detail::FieldRadius>(
    detail::FieldRadius(minRadius, maxRadius, range))
    .Invoke(this->CylIds, this->Radii, field.GetData().ResetTypes(vtkm::TypeListFieldScalar()));
}


vtkm::cont::ArrayHandle<vtkm::Id3> CylinderExtractor::GetCylIds()
{
  return this->CylIds;
}

vtkm::cont::ArrayHandle<vtkm::Float32> CylinderExtractor::GetRadii()
{
  return this->Radii;
}

vtkm::Id CylinderExtractor::GetNumberOfCylinders() const
{
  return this->CylIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
