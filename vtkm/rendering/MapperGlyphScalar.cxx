//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperGlyphScalar.h>

#include <vtkm/cont/Timer.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/GlyphExtractor.h>
#include <vtkm/rendering/raytracing/GlyphIntersector.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>

namespace vtkm
{
namespace rendering
{
namespace
{
// Packed frame buffer value with color set as black and depth as 1.0f
constexpr vtkm::Int64 ClearValue = 0x3F800000000000FF;

union PackedValue {
  struct PackedFloats
  {
    vtkm::Float32 Color;
    vtkm::Float32 Depth;
  } Floats;
  struct PackedInts
  {
    vtkm::UInt32 Color;
    vtkm::UInt32 Depth;
  } Ints;
  vtkm::Int64 Raw;
}; // union PackedValue

VTKM_EXEC_CONT
vtkm::UInt32 ScaleColorComponent(vtkm::Float32 c)
{
  vtkm::Int32 t = vtkm::Int32(c * 256.0f);
  return vtkm::UInt32(t < 0 ? 0 : (t > 255 ? 255 : t));
}

VTKM_EXEC_CONT
vtkm::UInt32 PackColor(vtkm::Float32 r, vtkm::Float32 g, vtkm::Float32 b, vtkm::Float32 a);

VTKM_EXEC_CONT
vtkm::UInt32 PackColor(const vtkm::Vec4f_32& color)
{
  return PackColor(color[0], color[1], color[2], color[3]);
}

VTKM_EXEC_CONT
vtkm::UInt32 PackColor(vtkm::Float32 r, vtkm::Float32 g, vtkm::Float32 b, vtkm::Float32 a)
{
  vtkm::UInt32 packed = (ScaleColorComponent(r) << 24);
  packed |= (ScaleColorComponent(g) << 16);
  packed |= (ScaleColorComponent(b) << 8);
  packed |= ScaleColorComponent(a);
  return packed;
}

VTKM_EXEC_CONT
void UnpackColor(vtkm::UInt32 color,
                 vtkm::Float32& r,
                 vtkm::Float32& g,
                 vtkm::Float32& b,
                 vtkm::Float32& a);

VTKM_EXEC_CONT
void UnpackColor(vtkm::UInt32 packedColor, vtkm::Vec4f_32& color)
{
  UnpackColor(packedColor, color[0], color[1], color[2], color[3]);
}

VTKM_EXEC_CONT
void UnpackColor(vtkm::UInt32 color,
                 vtkm::Float32& r,
                 vtkm::Float32& g,
                 vtkm::Float32& b,
                 vtkm::Float32& a)
{
  r = vtkm::Float32((color & 0xFF000000) >> 24) / 255.0f;
  g = vtkm::Float32((color & 0x00FF0000) >> 16) / 255.0f;
  b = vtkm::Float32((color & 0x0000FF00) >> 8) / 255.0f;
  a = vtkm::Float32((color & 0x000000FF)) / 255.0f;
}

class PackIntoFrameBuffer : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT
  PackIntoFrameBuffer() {}

  VTKM_EXEC
  void operator()(const vtkm::Vec4f_32& color,
                  const vtkm::Float32& depth,
                  vtkm::Int64& outValue) const
  {
    PackedValue packed;
    packed.Ints.Color = PackColor(color);
    packed.Floats.Depth = depth;
    outValue = packed.Raw;
  }
}; //class PackIntoFrameBuffer

class UnpackFromFrameBuffer : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  UnpackFromFrameBuffer() {}

  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VTKM_EXEC void operator()(const vtkm::Int64& packedValue,
                            ColorBufferPortal& colorBuffer,
                            DepthBufferPortal& depthBuffer,
                            const vtkm::Id& index) const
  {
    PackedValue packed;
    packed.Raw = packedValue;
    float depth = packed.Floats.Depth;
    if (depth <= depthBuffer.Get(index))
    {
      vtkm::Vec4f_32 color;
      UnpackColor(packed.Ints.Color, color);
      colorBuffer.Set(index, color);
      depthBuffer.Set(index, depth);
    }
  }
}; // class UnpackFromFrameBuffer

class GetNormalizedScalars : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointIds, FieldOut normalizedScalars, WholeArrayIn field);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT GetNormalizedScalars(vtkm::Float32 minScalar, vtkm::Float32 maxScalar)
    : MinScalar(minScalar)
  {
    if (minScalar >= maxScalar)
    {
      this->InverseScalarDelta = 0.0f;
    }
    else
    {
      this->InverseScalarDelta = 1.0f / (maxScalar - minScalar);
    }
  }

  template <typename FieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            vtkm::Float32& normalizedScalar,
                            const FieldPortalType& field) const
  {
    normalizedScalar = static_cast<vtkm::Float32>(field.Get(pointId));
    normalizedScalar = (normalizedScalar - this->MinScalar) * this->InverseScalarDelta;
  }

private:
  vtkm::Float32 MinScalar;
  vtkm::Float32 InverseScalarDelta;
}; // class GetNormalizedScalars

class BillboardGlyphPlotter : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointIds,
                                FieldIn sizes,
                                FieldIn normalizedScalar,
                                WholeArrayIn coords,
                                WholeArrayIn colorMap,
                                AtomicArrayInOut frameBuffer);

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  VTKM_CONT
  BillboardGlyphPlotter(const vtkm::Matrix<vtkm::Float32, 4, 4>& worldToProjection,
                        vtkm::Id width,
                        vtkm::Id height,
                        vtkm::Float32 projectionOffset)
    : WorldToProjection(worldToProjection)
    , Width(width)
    , Height(height)
    , ProjectionOffset(projectionOffset)
  {
  }

  template <typename CoordinatesPortal, typename ColorMapPortal, typename FrameBuffer>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            const vtkm::Float32& size,
                            const vtkm::Float32& normalizedScalar,
                            const CoordinatesPortal& coordsPortal,
                            const ColorMapPortal& colorMap,
                            FrameBuffer& frameBuffer) const
  {
    vtkm::Vec3f_32 point = static_cast<vtkm::Vec3f_32>(coordsPortal.Get(pointId));
    point = this->TransformWorldToViewport(point);
    vtkm::Vec4f_32 color = this->GetColor(normalizedScalar, colorMap);

    vtkm::Float32 halfSize = size / 2.0f;
    vtkm::Id x1 = static_cast<vtkm::Id>(vtkm::Round(point[0] - halfSize));
    vtkm::Id x2 = static_cast<vtkm::Id>(vtkm::Round(point[0] + halfSize));
    vtkm::Id y1 = static_cast<vtkm::Id>(vtkm::Round(point[1] - halfSize));
    vtkm::Id y2 = static_cast<vtkm::Id>(vtkm::Round(point[1] + halfSize));
    vtkm::Float32 depth = point[2];

    for (vtkm::Id x = x1; x <= x2; ++x)
    {
      for (vtkm::Id y = y1; y <= y2; ++y)
      {
        this->SetColor(x, y, depth, color, frameBuffer);
      }
    }
  }

private:
  VTKM_EXEC
  vtkm::Vec3f_32 TransformWorldToViewport(const vtkm::Vec3f_32& point) const
  {
    vtkm::Vec4f_32 temp(point[0], point[1], point[2], 1.0f);
    vtkm::Vec3f_32 result;
    temp = vtkm::MatrixMultiply(this->WorldToProjection, temp);
    for (vtkm::IdComponent i = 0; i < 3; ++i)
    {
      result[i] = temp[i] / temp[3];
    }
    result[0] = (result[0] * 0.5f + 0.5f) * vtkm::Float32(this->Width);
    result[1] = (result[1] * 0.5f + 0.5f) * vtkm::Float32(this->Height);
    result[2] = result[2] * 0.5f + 0.5f;
    // Offset the point to a bit towards the camera. This is to ensure that the front faces of
    // the wireframe wins the z-depth check against the surface render, and is in addition to the
    // existing camera space offset.
    result[2] -= this->ProjectionOffset;
    return result;
  }

  template <typename ColorMapPortal>
  VTKM_EXEC vtkm::Vec4f_32 GetColor(vtkm::Float32 normalizedScalar,
                                    const ColorMapPortal& colorMap) const
  {
    vtkm::Id colorMapSize = colorMap.GetNumberOfValues() - 1;
    vtkm::Id colorIdx = static_cast<vtkm::Id>(normalizedScalar * colorMapSize);
    colorIdx = vtkm::Min(colorMapSize, vtkm::Max(vtkm::Id(0), colorIdx));
    return colorMap.Get(colorIdx);
  }

  template <typename FrameBuffer>
  VTKM_EXEC void SetColor(vtkm::Id x,
                          vtkm::Id y,
                          vtkm::Float32 depth,
                          const vtkm::Vec4f_32& color,
                          FrameBuffer& frameBuffer) const
  {
    if (x < 0 || x >= this->Width || y < 0 || y >= this->Height)
    {
      return;
    }

    vtkm::Id index = y * this->Width + x;
    PackedValue current, next;
    current.Raw = ClearValue;
    next.Floats.Depth = depth;

    vtkm::Vec4f_32 currentColor;
    do
    {
      UnpackColor(current.Ints.Color, currentColor);
      next.Ints.Color = PackColor(color);
      frameBuffer.CompareExchange(index, &current.Raw, next.Raw);
    } while (current.Floats.Depth > next.Floats.Depth);
  }

  const vtkm::Matrix<vtkm::Float32, 4, 4> WorldToProjection;
  const vtkm::Id Width;
  const vtkm::Id Height;
  const vtkm::Float32 ProjectionOffset;
}; // class BillboardGlyphPlotter

}

MapperGlyphScalar::MapperGlyphScalar()
  : MapperGlyphBase()
  , GlyphType(vtkm::rendering::GlyphType::Sphere)
{
}

MapperGlyphScalar::~MapperGlyphScalar() {}

vtkm::rendering::GlyphType MapperGlyphScalar::GetGlyphType() const
{
  return this->GlyphType;
}

void MapperGlyphScalar::SetGlyphType(vtkm::rendering::GlyphType glyphType)
{
  if (!(glyphType == vtkm::rendering::GlyphType::Axes ||
        glyphType == vtkm::rendering::GlyphType::Cube ||
        glyphType == vtkm::rendering::GlyphType::Quad ||
        glyphType == vtkm::rendering::GlyphType::Sphere))
  {
    throw vtkm::cont::ErrorBadValue("MapperGlyphScalar: bad glyph type");
  }

  this->GlyphType = glyphType;
}

void MapperGlyphScalar::RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                                    const vtkm::cont::CoordinateSystem& coords,
                                    const vtkm::cont::Field& scalarField,
                                    const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                    const vtkm::rendering::Camera& camera,
                                    const vtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  vtkm::rendering::raytracing::RayTracer tracer;
  tracer.Clear();

  logger->OpenLogEntry("mapper_glyph_scalar");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;

  vtkm::Bounds coordBounds = coords.GetBounds();
  vtkm::Float32 baseSize = this->BaseSize;
  if (baseSize == -1.f)
  {
    // set a default size
    vtkm::Float64 lx = coordBounds.X.Length();
    vtkm::Float64 ly = coordBounds.Y.Length();
    vtkm::Float64 lz = coordBounds.Z.Length();
    vtkm::Float64 mag = vtkm::Sqrt(lx * lx + ly * ly + lz * lz);
    if (this->GlyphType == vtkm::rendering::GlyphType::Quad)
    {
      baseSize = 20.0f;
    }
    else
    {
      // same as used in vtk ospray
      constexpr vtkm::Float64 heuristic = 500.;
      baseSize = static_cast<vtkm::Float32>(mag / heuristic);
    }
  }

  vtkm::rendering::raytracing::GlyphExtractor glyphExtractor;

  vtkm::cont::DataSet processedDataSet = this->FilterPoints(cellset, coords, scalarField);
  vtkm::cont::UnknownCellSet processedCellSet = processedDataSet.GetCellSet();
  vtkm::cont::CoordinateSystem processedCoords = processedDataSet.GetCoordinateSystem();
  vtkm::cont::Field processedField = processedDataSet.GetField(scalarField.GetName());

  if (this->ScaleByValue)
  {
    vtkm::Float32 minSize = baseSize - baseSize * this->ScaleDelta;
    vtkm::Float32 maxSize = baseSize + baseSize * this->ScaleDelta;
    if (this->UseNodes)
    {
      glyphExtractor.ExtractCoordinates(processedCoords, processedField, minSize, maxSize);
    }
    else
    {
      glyphExtractor.ExtractCells(processedCellSet, processedField, minSize, maxSize);
    }
  }
  else
  {
    if (this->UseNodes)
    {
      glyphExtractor.ExtractCoordinates(processedCoords, baseSize);
    }
    else
    {
      glyphExtractor.ExtractCells(processedCellSet, baseSize);
    }
  }

  if (this->GlyphType == vtkm::rendering::GlyphType::Quad)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> pointIds = glyphExtractor.GetPointIds();
    vtkm::cont::ArrayHandle<vtkm::Float32> sizes = glyphExtractor.GetSizes();

    vtkm::cont::ArrayHandle<vtkm::Float32> normalizedScalars;
    vtkm::Float32 rangeMin = static_cast<vtkm::Float32>(scalarRange.Min);
    vtkm::Float32 rangeMax = static_cast<vtkm::Float32>(scalarRange.Max);
    vtkm::cont::Invoker invoker;
    invoker(GetNormalizedScalars{ rangeMin, rangeMax },
            pointIds,
            normalizedScalars,
            vtkm::rendering::raytracing::GetScalarFieldArray(scalarField));

    vtkm::cont::ArrayHandle<vtkm::Int64> frameBuffer;
    invoker(PackIntoFrameBuffer{},
            this->Canvas->GetColorBuffer(),
            this->Canvas->GetDepthBuffer(),
            frameBuffer);

    vtkm::Range clippingRange = camera.GetClippingRange();
    vtkm::Float64 offset1 = (clippingRange.Max - clippingRange.Min) / 1.0e4;
    vtkm::Float64 offset2 = clippingRange.Min / 2.0;
    vtkm::Float32 offset = static_cast<vtkm::Float32>(vtkm::Min(offset1, offset2));
    vtkm::Matrix<vtkm::Float32, 4, 4> modelMatrix;
    vtkm::MatrixIdentity(modelMatrix);
    modelMatrix[2][3] = offset;
    vtkm::Matrix<vtkm::Float32, 4, 4> worldToCamera =
      vtkm::MatrixMultiply(modelMatrix, camera.CreateViewMatrix());
    vtkm::Matrix<vtkm::Float32, 4, 4> worldToProjection = vtkm::MatrixMultiply(
      camera.CreateProjectionMatrix(this->Canvas->GetWidth(), this->Canvas->GetHeight()),
      worldToCamera);
    vtkm::Float32 projectionOffset =
      vtkm::Max(0.03f / static_cast<vtkm::Float32>(camera.GetClippingRange().Length()), 1e-4f);
    invoker(
      BillboardGlyphPlotter{
        worldToProjection, this->Canvas->GetWidth(), this->Canvas->GetHeight(), projectionOffset },
      pointIds,
      sizes,
      normalizedScalars,
      coords,
      this->ColorMap,
      frameBuffer);

    timer.Start();
    invoker(UnpackFromFrameBuffer{},
            frameBuffer,
            this->Canvas->GetColorBuffer(),
            this->Canvas->GetDepthBuffer());
  }
  else
  {
    vtkm::Bounds shapeBounds;
    if (glyphExtractor.GetNumberOfGlyphs() > 0)
    {
      auto glyphIntersector = std::make_shared<raytracing::GlyphIntersector>(this->GlyphType);
      glyphIntersector->SetData(
        processedCoords, glyphExtractor.GetPointIds(), glyphExtractor.GetSizes());
      tracer.AddShapeIntersector(glyphIntersector);
      shapeBounds.Include(glyphIntersector->GetShapeBounds());
    }

    //
    // Create rays
    //
    vtkm::Int32 width = (vtkm::Int32)this->Canvas->GetWidth();
    vtkm::Int32 height = (vtkm::Int32)this->Canvas->GetHeight();

    vtkm::rendering::raytracing::Camera RayCamera;
    vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;

    RayCamera.SetParameters(camera, width, height);

    RayCamera.CreateRays(Rays, shapeBounds);
    Rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(Rays, camera, *this->Canvas);

    tracer.SetField(processedField, scalarRange);
    tracer.GetCamera() = RayCamera;
    tracer.SetColorMap(this->ColorMap);
    tracer.Render(Rays);

    timer.Start();
    this->Canvas->WriteToCanvas(Rays, Rays.Buffers.at(0).Buffer, camera);
  }

  if (this->CompositeBackground)
  {
    this->Canvas->BlendBackground();
  }
  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

vtkm::rendering::Mapper* MapperGlyphScalar::NewCopy() const
{
  return new vtkm::rendering::MapperGlyphScalar(*this);
}
}
} // vtkm::rendering
