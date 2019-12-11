//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/raytracing/SphereExtractor.h>

#include <vtkm/cont/Algorithm.h>
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

class CountPoints : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  VTKM_CONT
  CountPoints() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  VTKM_EXEC
  void operator()(vtkm::CellShapeTagGeneric shapeType, vtkm::Id& points) const
  {
    if (shapeType.Id == vtkm::CELL_SHAPE_VERTEX)
      points = 1;
    else
      points = 0;
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

}; // ClassCountPoints

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
                            const VecType& vtkmNotUsed(cellIndices),
                            const vtkm::Id& cellId,
                            OutputPortal& outputIndices) const
  {

    if (shapeType.Id == vtkm::CELL_SHAPE_VERTEX)
    {
      outputIndices.Set(pointOffset, cellId);
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
  void operator()(vtkm::Id& index, const vtkm::Id& idx) const { index = idx; }
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

void SphereExtractor::ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                                         const vtkm::Float32 radius)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformRadius(radius);
}

void SphereExtractor::ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                                         const vtkm::cont::Field& field,
                                         const vtkm::Float32 minRadius,
                                         const vtkm::Float32 maxRadius)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void SphereExtractor::ExtractCells(const vtkm::cont::DynamicCellSet& cells,
                                   const vtkm::Float32 radius)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformRadius(radius);
}
void SphereExtractor::ExtractCells(const vtkm::cont::DynamicCellSet& cells,
                                   const vtkm::cont::Field& field,
                                   const vtkm::Float32 minRadius,
                                   const vtkm::Float32 maxRadius)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void SphereExtractor::SetUniformRadius(const vtkm::Float32 radius)
{
  const vtkm::Id size = this->PointIds.GetNumberOfValues();
  Radii.Allocate(size);

  vtkm::cont::ArrayHandleConstant<vtkm::Float32> radiusHandle(radius, size);
  vtkm::cont::Algorithm::Copy(radiusHandle, Radii);
}

void SphereExtractor::SetPointIdsFromCoords(const vtkm::cont::CoordinateSystem& coords)
{
  vtkm::Id size = coords.GetNumberOfPoints();
  this->PointIds.Allocate(size);
  vtkm::worklet::DispatcherMapField<detail::Iterator>(detail::Iterator()).Invoke(this->PointIds);
}

void SphereExtractor::SetPointIdsFromCells(const vtkm::cont::DynamicCellSet& cells)
{
  using SingleType = vtkm::cont::CellSetSingleType<>;
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
    vtkm::worklet::DispatcherMapTopology<detail::CountPoints>(detail::CountPoints())
      .Invoke(cells, points);

    vtkm::Id totalPoints = 0;
    totalPoints = vtkm::cont::Algorithm::Reduce(points, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
    vtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    PointIds.Allocate(totalPoints);

    vtkm::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cells, cellOffsets, this->PointIds);
  }
  else if (cells.IsSameType(SingleType()))
  {
    SingleType pointCells = cells.Cast<SingleType>();
    vtkm::UInt8 shape_id = pointCells.GetCellShape(0);
    if (shape_id == vtkm::CELL_SHAPE_VERTEX)
    {
      this->PointIds.Allocate(numCells);
      vtkm::worklet::DispatcherMapField<detail::Iterator>(detail::Iterator())
        .Invoke(this->PointIds);
    }
  }
}

void SphereExtractor::SetVaryingRadius(const vtkm::Float32 minRadius,
                                       const vtkm::Float32 maxRadius,
                                       const vtkm::cont::Field& field)
{
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw vtkm::cont::ErrorBadValue("Sphere Extractor: scalar field must have one component");
  }

  vtkm::Range range = rangeArray.GetPortalConstControl().Get(0);

  Radii.Allocate(this->PointIds.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<detail::FieldRadius>(
    detail::FieldRadius(minRadius, maxRadius, range))
    .Invoke(this->PointIds, this->Radii, field.GetData().ResetTypes(vtkm::TypeListFieldScalar()));
}

vtkm::cont::ArrayHandle<vtkm::Id> SphereExtractor::GetPointIds()
{
  return this->PointIds;
}

vtkm::cont::ArrayHandle<vtkm::Float32> SphereExtractor::GetRadii()
{
  return this->Radii;
}

vtkm::Id SphereExtractor::GetNumberOfSpheres() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
