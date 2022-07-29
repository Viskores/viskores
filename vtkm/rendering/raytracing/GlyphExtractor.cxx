//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/rendering/raytracing/GlyphExtractor.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace
{

class CountPoints : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  VTKM_CONT
  CountPoints() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  template <typename ShapeType>
  VTKM_EXEC void operator()(ShapeType shape, vtkm::Id& points) const
  {
    points = (shape.Id == vtkm::CELL_SHAPE_VERTEX) ? 1 : 0;
  }
}; // class CountPoints

class Pointify : public vtkm::worklet::WorkletVisitCellsWithPoints
{

public:
  VTKM_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

  template <typename ShapeType, typename VecType, typename OutputPortal>
  VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                            ShapeType shape,
                            const VecType& vtkmNotUsed(cellIndices),
                            const vtkm::Id& cellId,
                            OutputPortal& outputIndices) const
  {
    if (shape.Id == vtkm::CELL_SHAPE_VERTEX)
    {
      outputIndices.Set(pointOffset, cellId);
    }
  }
}; //class Pointify

class GetFieldSize : public vtkm::worklet::WorkletMapField
{
protected:
  // vtkm::Float64 is used to handle field values that are very small or very large
  // and could loose precision if vtkm::Float32 is used.
  vtkm::Float64 MinSize;
  vtkm::Float64 SizeDelta;
  vtkm::Float64 MinValue;
  vtkm::Float64 InverseDelta;

public:
  VTKM_CONT
  GetFieldSize(vtkm::Float64 minSize, vtkm::Float64 maxSize, vtkm::Range scalarRange)
    : MinSize(minSize)
    , SizeDelta(maxSize - minSize)
    , MinValue(scalarRange.Min)
  {
    vtkm::Float64 delta = scalarRange.Max - scalarRange.Min;
    if (delta != 0.)
      InverseDelta = 1. / (delta);
    else
      InverseDelta = 0.; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename ScalarPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            vtkm::Float32& size,
                            const ScalarPortalType& scalars) const
  {
    vtkm::Float64 scalar = vtkm::Float64(scalars.Get(pointId));
    vtkm::Float64 t = (scalar - this->MinValue) * this->InverseDelta;
    size = static_cast<vtkm::Float32>(this->MinSize + t * this->SizeDelta);
  }

}; //class GetFieldSize

} //namespace

GlyphExtractor::GlyphExtractor() = default;

void GlyphExtractor::ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                                        const vtkm::Float32 size)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformSize(size);
}

void GlyphExtractor::ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                                        const vtkm::cont::Field& field,
                                        const vtkm::Float32 minSize,
                                        const vtkm::Float32 maxSize)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractor::ExtractCells(const vtkm::cont::UnknownCellSet& cells, const vtkm::Float32 size)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformSize(size);
}
void GlyphExtractor::ExtractCells(const vtkm::cont::UnknownCellSet& cells,
                                  const vtkm::cont::Field& field,
                                  const vtkm::Float32 minSize,
                                  const vtkm::Float32 maxSize)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractor::SetUniformSize(const vtkm::Float32 size)
{
  const vtkm::Id numValues = this->PointIds.GetNumberOfValues();
  Sizes.AllocateAndFill(numValues, size);
}

void GlyphExtractor::SetPointIdsFromCoords(const vtkm::cont::CoordinateSystem& coords)
{
  vtkm::Id size = coords.GetNumberOfPoints();
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(size), this->PointIds);
}

void GlyphExtractor::SetPointIdsFromCells(const vtkm::cont::UnknownCellSet& cells)
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
  if (cells.CanConvert<vtkm::cont::CellSetExplicit<>>())
  {
    auto cellsExplicit = cells.AsCellSet<vtkm::cont::CellSetExplicit<>>();

    vtkm::cont::ArrayHandle<vtkm::Id> points;
    vtkm::worklet::DispatcherMapTopology<CountPoints>(CountPoints()).Invoke(cellsExplicit, points);

    vtkm::Id totalPoints = 0;
    totalPoints = vtkm::cont::Algorithm::Reduce(points, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
    vtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    PointIds.Allocate(totalPoints);

    vtkm::worklet::DispatcherMapTopology<Pointify>(Pointify())
      .Invoke(cellsExplicit, cellOffsets, this->PointIds);
  }
  else if (cells.CanConvert<SingleType>())
  {
    SingleType pointCells = cells.AsCellSet<SingleType>();
    vtkm::UInt8 shape_id = pointCells.GetCellShape(0);
    if (shape_id == vtkm::CELL_SHAPE_VERTEX)
    {
      vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(numCells), this->PointIds);
    }
  }
}

void GlyphExtractor::SetVaryingSize(const vtkm::Float32 minSize,
                                    const vtkm::Float32 maxSize,
                                    const vtkm::cont::Field& field)
{
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw vtkm::cont::ErrorBadValue("Glyph Extractor: scalar field must have one component");
  }

  vtkm::Range range = rangeArray.ReadPortal().Get(0);

  Sizes.Allocate(this->PointIds.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<GetFieldSize>(GetFieldSize(minSize, maxSize, range))
    .Invoke(this->PointIds, this->Sizes, vtkm::rendering::raytracing::GetScalarFieldArray(field));
}

vtkm::cont::ArrayHandle<vtkm::Id> GlyphExtractor::GetPointIds()
{
  return this->PointIds;
}

vtkm::cont::ArrayHandle<vtkm::Float32> GlyphExtractor::GetSizes()
{
  return this->Sizes;
}

vtkm::Id GlyphExtractor::GetNumberOfGlyphs() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
