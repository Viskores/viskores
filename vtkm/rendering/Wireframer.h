//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_rendering_Wireframer_h
#define vtk_m_rendering_Wireframer_h

#include <vtkm/Assert.h>
#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/exec/AtomicArray.h>
#include <vtkm/rendering/MatrixHelpers.h>
#include <vtkm/rendering/Triangulator.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace
{

using ColorMapHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
using IndicesHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 2>>;
using PackedFrameBufferHandle = vtkm::cont::ArrayHandle<vtkm::Int64>;

// Packed frame buffer value with color set as white and depth as 1.0f
const vtkm::Int64 ClearValue = 0x3F800000FFFFFFFF;

VTKM_EXEC_CONT
vtkm::Float32 IntegerPart(vtkm::Float32 x)
{
  return vtkm::Floor(x);
}

VTKM_EXEC_CONT
vtkm::Float32 FractionalPart(vtkm::Float32 x)
{
  return x - vtkm::Floor(x);
}

VTKM_EXEC_CONT
vtkm::Float32 ReverseFractionalPart(vtkm::Float32 x)
{
  return 1.0f - FractionalPart(x);
}

VTKM_EXEC_CONT
vtkm::UInt32 ComponentAsUInt32(vtkm::Float32 c)
{
  vtkm::Int32 t = vtkm::Int32(c * 256.0f);
  return vtkm::UInt32(t < 0 ? 0 : (t > 255 ? 255 : t));
}

VTKM_EXEC_CONT
vtkm::UInt32 PackColor(vtkm::Float32 r, vtkm::Float32 g, vtkm::Float32 b)
{
  vtkm::UInt32 packed = (ComponentAsUInt32(r) << 16);
  packed |= (ComponentAsUInt32(g) << 8);
  packed |= ComponentAsUInt32(b);
  return packed;
}

VTKM_EXEC_CONT
void UnpackColor(vtkm::UInt32 color, vtkm::Float32& r, vtkm::Float32& g, vtkm::Float32& b)
{
  r = vtkm::Float32((color & 0x00FF0000) >> 16) / 255.0f;
  g = vtkm::Float32((color & 0x0000FF00) >> 8) / 255.0f;
  b = vtkm::Float32((color & 0x000000FF)) / 255.0f;
}

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

struct DepthBufferCopy : public vtkm::worklet::WorkletMapField
{
  vtkm::UInt32 ClearColor;

  VTKM_CONT
  DepthBufferCopy(vtkm::UInt32 clearColor)
    : ClearColor(clearColor)
  {
  }

  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);

  VTKM_EXEC
  void operator()(const vtkm::Float32& depth, vtkm::Int64& outValue) const
  {
    PackedValue packed;
    packed.Ints.Color = ClearColor;
    packed.Floats.Depth = depth;
    outValue = packed.Raw;
  }
}; //struct DepthBufferCopy

template <typename DeviceTag>
class EdgePlotter : public vtkm::worklet::WorkletMapField
{
public:
  using AtomicPackedFrameBufferHandle = vtkm::exec::AtomicArray<vtkm::Int64, DeviceTag>;

  typedef void ControlSignature(FieldIn<>, WholeArrayIn<>, WholeArrayIn<Scalar>);
  typedef void ExecutionSignature(_1, _2, _3);
  using InputDomain = _1;

  VTKM_CONT
  EdgePlotter(const vtkm::Matrix<vtkm::Float32, 4, 4>& worldToProjection,
              vtkm::Id width,
              vtkm::Id height,
              const vtkm::Range& fieldRange,
              const ColorMapHandle& colorMap,
              const AtomicPackedFrameBufferHandle& frameBuffer,
              const vtkm::Range& clippingRange)
    : WorldToProjection(worldToProjection)
    , Width(width)
    , Height(height)
    , ColorMap(colorMap.PrepareForInput(DeviceTag()))
    , ColorMapSize(vtkm::Float32(colorMap.GetNumberOfValues() - 1))
    , FrameBuffer(frameBuffer)
    , FieldMin(vtkm::Float32(fieldRange.Min))
  {
    InverseFieldDelta = 1.0f / vtkm::Float32(fieldRange.Length());
    Offset = vtkm::Max(0.03f / vtkm::Float32(clippingRange.Length()), 0.000001f);
  }

  template <typename CoordinatesPortalType, typename ScalarFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Id, 2>& edgeIndices,
                            const CoordinatesPortalType& coordsPortal,
                            const ScalarFieldPortalType& fieldPortal) const
  {
    vtkm::Id point1Idx = edgeIndices[0];
    vtkm::Id point2Idx = edgeIndices[1];

    vtkm::Vec<vtkm::Float32, 3> point1 = coordsPortal.Get(edgeIndices[0]);
    vtkm::Vec<vtkm::Float32, 3> point2 = coordsPortal.Get(edgeIndices[1]);
    TransformWorldToViewport(point1);
    TransformWorldToViewport(point2);

    vtkm::Float32 x1 = point1[0];
    vtkm::Float32 y1 = point1[1];
    vtkm::Float32 z1 = point1[2];
    vtkm::Float32 x2 = point2[0];
    vtkm::Float32 y2 = point2[1];
    vtkm::Float32 z2 = point2[2];
    // If the line is steep, i.e., the height is greater than the width, then
    // transpose the co-ordinates to prevent "holes" in the line. This ensures
    // that we pick the co-ordinate which grows at a lesser rate than the other.
    bool transposed = vtkm::Abs(y2 - y1) > vtkm::Abs(x2 - x1);
    if (transposed)
    {
      std::swap(x1, y1);
      std::swap(x2, y2);
    }

    // Ensure we are always going from left to right
    if (x1 > x2)
    {
      std::swap(x1, x2);
      std::swap(y1, y2);
      std::swap(z1, z2);
    }

    vtkm::Float32 dx = x2 - x1;
    vtkm::Float32 dy = y2 - y1;
    vtkm::Float32 gradient = (dx == 0.0) ? 1.0f : (dy / dx);

    vtkm::Float32 xEnd = vtkm::Round(x1);
    vtkm::Float32 yEnd = y1 + gradient * (xEnd - x1);
    vtkm::Float32 xGap = ReverseFractionalPart(x1 + 0.5f);
    vtkm::Float32 xPxl1 = xEnd, yPxl1 = IntegerPart(yEnd);
    vtkm::Float64 point1Field = fieldPortal.Get(point1Idx);
    vtkm::Float64 point2Field = fieldPortal.Get(point2Idx);

    // Plot first endpoint
    vtkm::Vec<vtkm::Float32, 4> color = GetColor(point1Field);
    if (transposed)
    {
      Plot(yPxl1, xPxl1, z1, color, ReverseFractionalPart(yEnd) * xGap);
      Plot(yPxl1 + 1, xPxl1, z1, color, FractionalPart(yEnd) * xGap);
    }
    else
    {
      Plot(xPxl1, yPxl1, z1, color, ReverseFractionalPart(yEnd) * xGap);
      Plot(xPxl1, yPxl1 + 1, z1, color, FractionalPart(yEnd) * xGap);
    }

    vtkm::Float32 interY = yEnd + gradient;
    xEnd = vtkm::Round(x2);
    yEnd = y2 + gradient * (xEnd - x2);
    xGap = FractionalPart(x2 + 0.5f);
    vtkm::Float32 xPxl2 = xEnd, yPxl2 = IntegerPart(yEnd);

    // Plot second endpoint
    color = GetColor(point2Field);
    if (transposed)
    {
      Plot(yPxl2, xPxl2, z2, color, ReverseFractionalPart(yEnd) * xGap);
      Plot(yPxl2 + 1, xPxl2, z2, color, FractionalPart(yEnd) * xGap);
    }
    else
    {
      Plot(xPxl2, yPxl2, z2, color, ReverseFractionalPart(yEnd) * xGap);
      Plot(xPxl2, yPxl2 + 1, z2, color, FractionalPart(yEnd) * xGap);
    }

    // Plot rest of the line
    if (transposed)
    {
      for (vtkm::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        vtkm::Float32 t = IntegerPart(interY);
        vtkm::Float32 factor = (x - x1) / dx;
        vtkm::Float32 depth = vtkm::Lerp(z1, z2, factor);
        vtkm::Float64 fieldValue = vtkm::Lerp(point1Field, point2Field, factor);
        color = GetColor(fieldValue);
        Plot(t, x, depth, color, ReverseFractionalPart(interY));
        Plot(t + 1, x, depth, color, FractionalPart(interY));
        interY += gradient;
      }
    }
    else
    {
      for (vtkm::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        vtkm::Float32 t = IntegerPart(interY);
        vtkm::Float32 factor = (x - x1) / dx;
        vtkm::Float32 depth = vtkm::Lerp(z1, z2, factor);
        vtkm::Float64 fieldValue = vtkm::Lerp(point1Field, point2Field, factor);
        color = GetColor(fieldValue);
        Plot(x, t, depth, color, ReverseFractionalPart(interY));
        Plot(x, t + 1, depth, color, FractionalPart(interY));
        interY += gradient;
      }
    }
  }

private:
  using ColorMapPortalConst = typename ColorMapHandle::ExecutionTypes<DeviceTag>::PortalConst;

  VTKM_EXEC
  void TransformWorldToViewport(vtkm::Vec<vtkm::Float32, 3>& point) const
  {
    vtkm::Vec<vtkm::Float32, 4> temp(point[0], point[1], point[2], 1.0f);
    temp = vtkm::MatrixMultiply(WorldToProjection, temp);
    for (vtkm::IdComponent i = 0; i < 3; ++i)
    {
      point[i] = temp[i] / temp[3];
    }
    // Scale to canvas width and height
    point[0] = (point[0] * 0.5f + 0.5f) * vtkm::Float32(Width);
    point[1] = (point[1] * 0.5f + 0.5f) * vtkm::Float32(Height);
    // Convert from -1/+1 to 0/+1 range
    point[2] = point[2] * 0.5f + 0.5f;
    // Offset the point to a bit towards the camera. This is to ensure that the front faces of
    // the wireframe wins the z-depth check against the surface render.
    point[2] -= Offset;
  }

  VTKM_EXEC vtkm::Vec<vtkm::Float32, 4> GetColor(vtkm::Float64 fieldValue) const
  {
    vtkm::Int32 colorIdx =
      vtkm::Int32((vtkm::Float32(fieldValue) - FieldMin) * ColorMapSize * InverseFieldDelta);
    return ColorMap.Get(colorIdx);
  }

  VTKM_EXEC
  void Plot(vtkm::Float32 x,
            vtkm::Float32 y,
            vtkm::Float32 depth,
            const vtkm::Vec<vtkm::Float32, 4>& color,
            vtkm::Float32 intensity) const
  {
    vtkm::Id xi = static_cast<vtkm::Id>(x), yi = static_cast<vtkm::Id>(y);
    if (xi < 0 || xi >= Width || yi < 0 || yi >= Height)
    {
      return;
    }
    vtkm::Id index = yi * Width + xi;
    PackedValue current, next;
    current.Raw = ClearValue;
    next.Floats.Depth = depth;
    vtkm::Vec<vtkm::Float32, 3> blendedColor;
    vtkm::Vec<vtkm::Float32, 3> srcColor;
    do
    {
      UnpackColor(current.Ints.Color, srcColor[0], srcColor[1], srcColor[2]);
      vtkm::Float32 inverseIntensity = (1.0f - intensity);
      blendedColor[0] = color[0] * intensity + srcColor[0] * inverseIntensity;
      blendedColor[1] = color[1] * intensity + srcColor[1] * inverseIntensity;
      blendedColor[2] = color[2] * intensity + srcColor[2] * inverseIntensity;
      next.Ints.Color = PackColor(blendedColor[0], blendedColor[1], blendedColor[2]);
      current.Raw = FrameBuffer.CompareAndSwap(index, next.Raw, current.Raw);
    } while (current.Floats.Depth > next.Floats.Depth);
  }

  vtkm::Matrix<vtkm::Float32, 4, 4> WorldToProjection;
  vtkm::Id Width;
  vtkm::Id Height;
  ColorMapPortalConst ColorMap;
  vtkm::Float32 ColorMapSize;
  AtomicPackedFrameBufferHandle FrameBuffer;
  vtkm::Float32 FieldMin;
  vtkm::Float32 InverseFieldDelta;
  vtkm::Float32 Offset;
};

struct BufferConverter : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  BufferConverter(vtkm::Vec<vtkm::Float32, 4> backgroundColor)
    : BackgroundColor(backgroundColor)
  {
  }

  typedef void ControlSignature(FieldIn<>, ExecObject, ExecObject);
  typedef void ExecutionSignature(_1, _2, _3, WorkIndex);

  VTKM_EXEC
  void operator()(const vtkm::Int64& packedValue,
                  vtkm::exec::ExecutionWholeArray<vtkm::Float32>& depthBuffer,
                  vtkm::exec::ExecutionWholeArray<vtkm::Vec<vtkm::Float32, 4>>& colorBuffer,
                  const vtkm::Id& index) const
  {
    PackedValue packed;
    packed.Raw = packedValue;
    float depth = packed.Floats.Depth;
    if (depth <= depthBuffer.Get(index))
    {
      depthBuffer.Set(index, depth);
      vtkm::Vec<vtkm::Float32, 4> color;
      UnpackColor(packed.Ints.Color, color[0], color[1], color[2]);
      color[3] = 1.0f;
      colorBuffer.Set(index, color);
    }
    else
    {
      colorBuffer.Set(index, BackgroundColor);
    }
  }

private:
  vtkm::Vec<vtkm::Float32, 4> BackgroundColor;
};

} // namespace

class Wireframer
{
public:
  VTKM_CONT
  Wireframer(vtkm::rendering::Canvas* canvas, bool showInternalZones)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
  {
  }

  VTKM_CONT
  void SetCamera(const vtkm::rendering::Camera& camera) { this->Camera = camera; }

  VTKM_CONT
  void SetColorMap(const ColorMapHandle& colorMap) { this->ColorMap = colorMap; }

  VTKM_CONT
  void SetSolidDepthBuffer(const vtkm::cont::ArrayHandle<vtkm::Float32> depthBuffer)
  {
    this->SolidDepthBuffer = depthBuffer;
  }

  VTKM_CONT
  void SetData(const vtkm::cont::CoordinateSystem& coords,
               const IndicesHandle& endPointIndices,
               const vtkm::cont::Field& field,
               const vtkm::Range& fieldRange)
  {
    this->Bounds = coords.GetBounds();
    this->Coordinates = coords.GetData();
    this->PointIndices = endPointIndices;
    this->ScalarField = field;
    this->ScalarFieldRange = fieldRange;
  }

  VTKM_CONT
  void Render()
  {
    RenderWithDeviceFunctor functor(this);
    vtkm::cont::TryExecute(functor);
  }

private:
  template <typename DeviceTag>
  VTKM_CONT void RenderWithDevice(DeviceTag)
  {
    if (ScalarField.GetAssociation() != vtkm::cont::Field::ASSOC_POINTS)
    {
      throw vtkm::cont::ErrorBadValue("Field is not associated with points");
    }

    vtkm::Matrix<vtkm::Float32, 4, 4> WorldToProjection =
      vtkm::MatrixMultiply(Camera.CreateProjectionMatrix(Canvas->GetWidth(), Canvas->GetHeight()),
                           Camera.CreateViewMatrix());
    vtkm::Id width = static_cast<vtkm::Id>(Canvas->GetWidth());
    vtkm::Id height = static_cast<vtkm::Id>(Canvas->GetHeight());
    vtkm::Id pixelCount = width * height;
    FrameBuffer.PrepareForOutput(pixelCount, DeviceTag());

    vtkm::Vec<vtkm::Float32, 4> clearColor = Canvas->GetBackgroundColor().Components;
    vtkm::UInt32 packedClearColor = PackColor(clearColor[0], clearColor[1], clearColor[2]);
    if (ShowInternalZones)
    {
      using MemSet =
        typename vtkm::rendering::Triangulator<DeviceTag>::template MemSet<vtkm::Int64>;
      vtkm::Int64 clearValue = (static_cast<vtkm::Int64>(0x3F800000) << 32) | packedClearColor;
      MemSet memSet(clearValue);
      vtkm::worklet::DispatcherMapField<MemSet>(memSet).Invoke(FrameBuffer);
    }
    else
    {
      VTKM_ASSERT(SolidDepthBuffer.GetNumberOfValues() == pixelCount);
      DepthBufferCopy bufferCopy(packedClearColor);
      vtkm::worklet::DispatcherMapField<DepthBufferCopy>(bufferCopy)
        .Invoke(SolidDepthBuffer, FrameBuffer);
    }

    EdgePlotter<DeviceTag> plotter(WorldToProjection,
                                   width,
                                   height,
                                   ScalarFieldRange,
                                   ColorMap,
                                   FrameBuffer,
                                   Camera.GetClippingRange());
    vtkm::worklet::DispatcherMapField<EdgePlotter<DeviceTag>, DeviceTag>(plotter).Invoke(
      PointIndices, Coordinates, ScalarField.GetData());

    BufferConverter converter(clearColor);
    vtkm::worklet::DispatcherMapField<BufferConverter, DeviceTag>(converter).Invoke(
      FrameBuffer,
      vtkm::exec::ExecutionWholeArray<vtkm::Float32>(Canvas->GetDepthBuffer()),
      vtkm::exec::ExecutionWholeArray<vtkm::Vec<vtkm::Float32, 4>>(Canvas->GetColorBuffer()));
  }

  VTKM_CONT
  struct RenderWithDeviceFunctor
  {
    Wireframer* Renderer;

    RenderWithDeviceFunctor(Wireframer* renderer)
      : Renderer(renderer)
    {
    }

    template <typename DeviceTag>
    VTKM_CONT bool operator()(DeviceTag)
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);
      Renderer->RenderWithDevice(DeviceTag());
      return true;
    }
  };

  vtkm::Bounds Bounds;
  vtkm::rendering::Camera Camera;
  vtkm::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  ColorMapHandle ColorMap;
  vtkm::cont::DynamicArrayHandleCoordinateSystem Coordinates;
  IndicesHandle PointIndices;
  vtkm::cont::Field ScalarField;
  vtkm::Range ScalarFieldRange;
  vtkm::cont::ArrayHandle<vtkm::Float32> SolidDepthBuffer;
  PackedFrameBufferHandle FrameBuffer;
}; // class Wireframer
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_Wireframer_h
