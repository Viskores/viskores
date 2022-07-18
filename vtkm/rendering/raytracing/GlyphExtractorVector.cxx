//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/rendering/raytracing/GlyphExtractorVector.h>
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

struct MinFunctor
{
  template <typename VecType>
  VTKM_EXEC VecType operator()(const VecType& x, const VecType& y) const
  {
    return (vtkm::MagnitudeSquared(y) < vtkm::MagnitudeSquared(x)) ? y : x;
  }
};

struct MaxFunctor
{
  template <typename VecType>
  VTKM_EXEC VecType operator()(const VecType& x, const VecType& y) const
  {
    return (vtkm::MagnitudeSquared(x) < vtkm::MagnitudeSquared(y)) ? y : x;
  }
};

class GetFieldSize : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Float64 MinSize;
  vtkm::Float64 SizeDelta;
  vtkm::Float64 MinValueMagnitude;
  vtkm::Float64 InverseDelta;

public:
  VTKM_CONT
  GetFieldSize(vtkm::Float64 minSize,
               vtkm::Float64 maxSize,
               vtkm::Vec3f_64 minValue,
               vtkm::Vec3f_64 maxValue)
    : MinSize(minSize)
    , SizeDelta(maxSize - minSize)
  {
    MinValueMagnitude = vtkm::Magnitude(minValue);
    vtkm::Float64 minMag = vtkm::Magnitude(minValue);
    vtkm::Float64 maxMag = vtkm::Magnitude(maxValue);
    vtkm::Float64 delta = maxMag - minMag;
    if (delta != 0.)
      InverseDelta = 1. / (delta);
    else
      InverseDelta = 0.; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename FieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            vtkm::Vec3f_32& size,
                            const FieldPortalType& field) const
  {
    using ValueType = typename FieldPortalType::ValueType;

    ValueType fieldVal = field.Get(pointId);
    vtkm::Float64 fieldValMag = vtkm::Magnitude(fieldVal);
    vtkm::Normalize(fieldVal);
    vtkm::Float64 t = (fieldValMag - MinValueMagnitude) * InverseDelta;
    vtkm::Float64 sizeMag = MinSize + t * SizeDelta;
    vtkm::Vec3f_64 tempSize = fieldVal * sizeMag;

    size[0] = static_cast<vtkm::Float32>(tempSize[0]);
    size[1] = static_cast<vtkm::Float32>(tempSize[1]);
    size[2] = static_cast<vtkm::Float32>(tempSize[2]);
  }

}; //class GetFieldSize

class FieldMagnitude : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FieldMagnitude() {}

  typedef void ControlSignature(FieldIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename FieldPortalType, typename MagnitudeFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            const FieldPortalType& field,
                            MagnitudeFieldPortalType& magnitudeField) const
  {
    using FieldValueType = typename FieldPortalType::ValueType;

    FieldValueType fieldVal = field.Get(pointId);
    vtkm::Float32 fieldValMag = static_cast<vtkm::Float32>(vtkm::Magnitude(fieldVal));
    magnitudeField.Set(pointId, fieldValMag);
  }
}; //class FieldMagnitude

class UniformFieldMagnitude : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT
  UniformFieldMagnitude(vtkm::Float32 uniformMagnitude)
    : UniformMagnitude(uniformMagnitude)
  {
  }

  template <typename FieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            vtkm::Vec3f_32& size,
                            const FieldPortalType& field) const
  {
    vtkm::Vec3f_32 fieldVal = static_cast<vtkm::Vec3f_32>(field.Get(pointId));
    size = vtkm::Normal(fieldVal) * this->UniformMagnitude;
  }

  vtkm::Float32 UniformMagnitude;
}; //class UniformFieldMagnitude

} //namespace

GlyphExtractorVector::GlyphExtractorVector() = default;

void GlyphExtractorVector::ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                                              const vtkm::cont::Field& field,
                                              const vtkm::Float32 size)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformSize(size, field);
}

void GlyphExtractorVector::ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                                              const vtkm::cont::Field& field,
                                              const vtkm::Float32 minSize,
                                              const vtkm::Float32 maxSize)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractorVector::ExtractCells(const vtkm::cont::UnknownCellSet& cells,
                                        const vtkm::cont::Field& field,
                                        const vtkm::Float32 size)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformSize(size, field);
}
void GlyphExtractorVector::ExtractCells(const vtkm::cont::UnknownCellSet& cells,
                                        const vtkm::cont::Field& field,
                                        const vtkm::Float32 minSize,
                                        const vtkm::Float32 maxSize)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractorVector::SetUniformSize(const vtkm::Float32 size, const vtkm::cont::Field& field)
{
  this->ExtractMagnitudeField(field);

  this->Sizes.Allocate(this->PointIds.GetNumberOfValues());
  vtkm::cont::Invoker invoker;
  invoker(UniformFieldMagnitude(size),
          this->PointIds,
          this->Sizes,
          vtkm::rendering::raytracing::GetVec3FieldArray(field));
}

void GlyphExtractorVector::ExtractMagnitudeField(const vtkm::cont::Field& field)
{
  vtkm::cont::ArrayHandle<vtkm::Float32> magnitudeArray;
  magnitudeArray.Allocate(this->PointIds.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<FieldMagnitude>(FieldMagnitude())
    .Invoke(this->PointIds, vtkm::rendering::raytracing::GetVec3FieldArray(field), magnitudeArray);
  this->MagnitudeField = vtkm::cont::Field(field);
  this->MagnitudeField.SetData(magnitudeArray);
}

void GlyphExtractorVector::SetPointIdsFromCoords(const vtkm::cont::CoordinateSystem& coords)
{
  vtkm::Id size = coords.GetNumberOfPoints();
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(size), this->PointIds);
}

void GlyphExtractorVector::SetPointIdsFromCells(const vtkm::cont::UnknownCellSet& cells)
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

void GlyphExtractorVector::SetVaryingSize(const vtkm::Float32 minSize,
                                          const vtkm::Float32 maxSize,
                                          const vtkm::cont::Field& field)
{
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 3)
  {
    throw vtkm::cont::ErrorBadValue(
      "Glyph Extractor Vector: vector field must have three components");
  }

  using Vec3f_32Handle = vtkm::cont::ArrayHandle<vtkm::Vec3f_32>;
  using Vec3f_64Handle = vtkm::cont::ArrayHandle<vtkm::Vec3f_64>;
  vtkm::cont::UnknownArrayHandle fieldUnknownHandle = field.GetData();
  vtkm::Vec3f_32 minFieldValue, maxFieldValue;

  if (fieldUnknownHandle.CanConvert<Vec3f_64Handle>())
  {
    Vec3f_64Handle fieldArray;
    field.GetData().AsArrayHandle(fieldArray);
    vtkm::Vec3f_64 initVal = vtkm::cont::ArrayGetValue(0, fieldArray);
    minFieldValue =
      static_cast<vtkm::Vec3f_32>(vtkm::cont::Algorithm::Reduce(fieldArray, initVal, MinFunctor()));
    maxFieldValue =
      static_cast<vtkm::Vec3f_32>(vtkm::cont::Algorithm::Reduce(fieldArray, initVal, MaxFunctor()));
  }
  else
  {
    Vec3f_32Handle fieldArray;
    field.GetData().AsArrayHandle(fieldArray);
    vtkm::Vec3f_32 initVal = vtkm::cont::ArrayGetValue(0, fieldArray);
    minFieldValue =
      static_cast<vtkm::Vec3f_32>(vtkm::cont::Algorithm::Reduce(fieldArray, initVal, MinFunctor()));
    maxFieldValue =
      static_cast<vtkm::Vec3f_32>(vtkm::cont::Algorithm::Reduce(fieldArray, initVal, MaxFunctor()));
  }

  this->ExtractMagnitudeField(field);

  this->Sizes.Allocate(this->PointIds.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<GetFieldSize>(
    GetFieldSize(minSize, maxSize, minFieldValue, maxFieldValue))
    .Invoke(this->PointIds, this->Sizes, vtkm::rendering::raytracing::GetVec3FieldArray(field));
}

vtkm::cont::ArrayHandle<vtkm::Id> GlyphExtractorVector::GetPointIds()
{
  return this->PointIds;
}

vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> GlyphExtractorVector::GetSizes()
{
  return this->Sizes;
}

vtkm::cont::Field GlyphExtractorVector::GetMagnitudeField()
{
  return this->MagnitudeField;
}

vtkm::Id GlyphExtractorVector::GetNumberOfGlyphs() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
