//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_Wireframer_h
#define vtk_m_rendering_Wireframer_h

#include <vtkm/Assert.h>
#include <vtkm/Math.h>
#include <vtkm/Swap.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/AtomicArray.h>
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

using ColorMapHandle = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
using IndicesHandle = vtkm::cont::ArrayHandle<vtkm::Id2>;
using PackedFrameBufferHandle = vtkm::cont::ArrayHandle<vtkm::Int64>;

// Depth value of 1.0f
const vtkm::Int64 ClearDepth = 0x3F800000;
// Packed frame buffer value with color set as black and depth as 1.0f
const vtkm::Int64 ClearValue = 0x3F800000000000FF;

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

struct CopyIntoFrameBuffer : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT
  CopyIntoFrameBuffer() {}

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
}; //struct CopyIntoFrameBuffer

template <typename DeviceTag>
class EdgePlotter : public vtkm::worklet::WorkletMapField
{
public:
  using AtomicPackedFrameBufferHandle =
    vtkm::exec::AtomicArrayExecutionObject<vtkm::Int64, DeviceTag>;
  using AtomicPackedFrameBuffer = vtkm::cont::AtomicArray<vtkm::Int64>;

  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  VTKM_CONT
  EdgePlotter(const vtkm::Matrix<vtkm::Float32, 4, 4>& worldToProjection,
              vtkm::Id width,
              vtkm::Id height,
              vtkm::Id subsetWidth,
              vtkm::Id subsetHeight,
              vtkm::Id xOffset,
              vtkm::Id yOffset,
              bool assocPoints,
              const vtkm::Range& fieldRange,
              const ColorMapHandle& colorMap,
              const AtomicPackedFrameBuffer& frameBuffer,
              const vtkm::Range& clippingRange)
    : WorldToProjection(worldToProjection)
    , Width(width)
    , Height(height)
    , SubsetWidth(subsetWidth)
    , SubsetHeight(subsetHeight)
    , XOffset(xOffset)
    , YOffset(yOffset)
    , AssocPoints(assocPoints)
    , ColorMap(colorMap.PrepareForInput(DeviceTag()))
    , ColorMapSize(vtkm::Float32(colorMap.GetNumberOfValues() - 1))
    , FrameBuffer(frameBuffer.PrepareForExecution(DeviceTag()))
    , FieldMin(vtkm::Float32(fieldRange.Min))
  {
    InverseFieldDelta = 1.0f / vtkm::Float32(fieldRange.Length());
    Offset = vtkm::Max(0.03f / vtkm::Float32(clippingRange.Length()), 0.0001f);
  }

  template <typename CoordinatesPortalType, typename ScalarFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id2& edgeIndices,
                            const CoordinatesPortalType& coordsPortal,
                            const ScalarFieldPortalType& fieldPortal) const
  {
    vtkm::Id point1Idx = edgeIndices[0];
    vtkm::Id point2Idx = edgeIndices[1];

    vtkm::Vec3f_32 point1 = coordsPortal.Get(edgeIndices[0]);
    vtkm::Vec3f_32 point2 = coordsPortal.Get(edgeIndices[1]);

    TransformWorldToViewport(point1);
    TransformWorldToViewport(point2);

    vtkm::Float32 x1 = vtkm::Round(point1[0]);
    vtkm::Float32 y1 = vtkm::Round(point1[1]);
    vtkm::Float32 z1 = point1[2];
    vtkm::Float32 x2 = vtkm::Round(point2[0]);
    vtkm::Float32 y2 = vtkm::Round(point2[1]);
    vtkm::Float32 z2 = point2[2];
    // If the line is steep, i.e., the height is greater than the width, then
    // transpose the co-ordinates to prevent "holes" in the line. This ensures
    // that we pick the co-ordinate which grows at a lesser rate than the other.
    bool transposed = vtkm::Abs(y2 - y1) > vtkm::Abs(x2 - x1);
    if (transposed)
    {
      vtkm::Swap(x1, y1);
      vtkm::Swap(x2, y2);
    }

    // Ensure we are always going from left to right
    if (x1 > x2)
    {
      vtkm::Swap(x1, x2);
      vtkm::Swap(y1, y2);
      vtkm::Swap(z1, z2);
    }

    vtkm::Float32 dx = x2 - x1;
    vtkm::Float32 dy = y2 - y1;
    vtkm::Float32 gradient = (dx == 0.0) ? 1.0f : (dy / dx);

    vtkm::Float32 xEnd = vtkm::Round(x1);
    vtkm::Float32 yEnd = y1 + gradient * (xEnd - x1);
    vtkm::Float32 xPxl1 = xEnd, yPxl1 = IntegerPart(yEnd);
    vtkm::Float32 zPxl1 = vtkm::Lerp(z1, z2, (xPxl1 - x1) / dx);
    vtkm::Float64 point1Field = fieldPortal.Get(point1Idx);
    vtkm::Float64 point2Field;
    if (AssocPoints)
    {
      point2Field = fieldPortal.Get(point2Idx);
    }
    else
    {
      // cell associated field has a solid line color
      point2Field = point1Field;
    }

    // Plot first endpoint
    vtkm::Vec4f_32 color = GetColor(point1Field);
    if (transposed)
    {
      Plot(yPxl1, xPxl1, zPxl1, color, 1.0f);
    }
    else
    {
      Plot(xPxl1, yPxl1, zPxl1, color, 1.0f);
    }

    vtkm::Float32 interY = yEnd + gradient;
    xEnd = vtkm::Round(x2);
    yEnd = y2 + gradient * (xEnd - x2);
    vtkm::Float32 xPxl2 = xEnd, yPxl2 = IntegerPart(yEnd);
    vtkm::Float32 zPxl2 = vtkm::Lerp(z1, z2, (xPxl2 - x1) / dx);

    // Plot second endpoint
    color = GetColor(point2Field);
    if (transposed)
    {
      Plot(yPxl2, xPxl2, zPxl2, color, 1.0f);
    }
    else
    {
      Plot(xPxl2, yPxl2, zPxl2, color, 1.0f);
    }

    // Plot rest of the line
    if (transposed)
    {
      for (vtkm::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        vtkm::Float32 t = IntegerPart(interY);
        vtkm::Float32 factor = (x - x1) / dx;
        vtkm::Float32 depth = vtkm::Lerp(zPxl1, zPxl2, factor);
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
        vtkm::Float32 depth = vtkm::Lerp(zPxl1, zPxl2, factor);
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
  void TransformWorldToViewport(vtkm::Vec3f_32& point) const
  {
    vtkm::Vec4f_32 temp(point[0], point[1], point[2], 1.0f);
    temp = vtkm::MatrixMultiply(WorldToProjection, temp);
    for (vtkm::IdComponent i = 0; i < 3; ++i)
    {
      point[i] = temp[i] / temp[3];
    }
    // Scale to canvas width and height
    point[0] = (point[0] * 0.5f + 0.5f) * vtkm::Float32(SubsetWidth) + vtkm::Float32(XOffset);
    point[1] = (point[1] * 0.5f + 0.5f) * vtkm::Float32(SubsetHeight) + vtkm::Float32(YOffset);
    // Convert from -1/+1 to 0/+1 range
    point[2] = point[2] * 0.5f + 0.5f;
    // Offset the point to a bit towards the camera. This is to ensure that the front faces of
    // the wireframe wins the z-depth check against the surface render, and is in addition to the
    // existing camera space offset.
    point[2] -= Offset;
  }

  VTKM_EXEC vtkm::Vec4f_32 GetColor(vtkm::Float64 fieldValue) const
  {
    vtkm::Int32 colorIdx =
      vtkm::Int32((vtkm::Float32(fieldValue) - FieldMin) * ColorMapSize * InverseFieldDelta);
    colorIdx = vtkm::Min(vtkm::Int32(ColorMap.GetNumberOfValues() - 1), vtkm::Max(0, colorIdx));
    return ColorMap.Get(colorIdx);
  }

  VTKM_EXEC
  void Plot(vtkm::Float32 x,
            vtkm::Float32 y,
            vtkm::Float32 depth,
            const vtkm::Vec4f_32& color,
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
    vtkm::Vec4f_32 blendedColor;
    vtkm::Vec4f_32 srcColor;
    do
    {
      UnpackColor(current.Ints.Color, srcColor);
      vtkm::Float32 inverseIntensity = (1.0f - intensity);
      vtkm::Float32 alpha = srcColor[3] * inverseIntensity;
      blendedColor[0] = color[0] * intensity + srcColor[0] * alpha;
      blendedColor[1] = color[1] * intensity + srcColor[1] * alpha;
      blendedColor[2] = color[2] * intensity + srcColor[2] * alpha;
      blendedColor[3] = alpha + intensity;
      next.Ints.Color = PackColor(blendedColor);
      current.Raw = FrameBuffer.CompareAndSwap(index, next.Raw, current.Raw);
    } while (current.Floats.Depth > next.Floats.Depth);
  }

  vtkm::Matrix<vtkm::Float32, 4, 4> WorldToProjection;
  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::Id SubsetWidth;
  vtkm::Id SubsetHeight;
  vtkm::Id XOffset;
  vtkm::Id YOffset;
  bool AssocPoints;
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
  BufferConverter() {}

  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);

  template <typename DepthBufferPortalType, typename ColorBufferPortalType>
  VTKM_EXEC void operator()(const vtkm::Int64& packedValue,
                            DepthBufferPortalType& depthBuffer,
                            ColorBufferPortalType& colorBuffer,
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
};

} // namespace

class Wireframer
{
public:
  VTKM_CONT
  Wireframer(vtkm::rendering::Canvas* canvas, bool showInternalZones, bool isOverlay)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
    , IsOverlay(isOverlay)
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
    this->Coordinates = coords;
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

    // The wireframe should appear on top of any prerendered data, and hide away the internal
    // zones if `ShowInternalZones` is set to false. Since the prerendered data (or the solid
    // depth buffer) could cause z-fighting with the wireframe, we will offset all the edges in Z
    // by a small amount, proportional to distance between the near and far camera planes, in the
    // camera space.
    vtkm::Range clippingRange = Camera.GetClippingRange();
    vtkm::Float64 offset1 = (clippingRange.Max - clippingRange.Min) / 1.0e4;
    vtkm::Float64 offset2 = clippingRange.Min / 2.0;
    vtkm::Float32 offset = static_cast<vtkm::Float32>(vtkm::Min(offset1, offset2));
    vtkm::Matrix<vtkm::Float32, 4, 4> modelMatrix;
    vtkm::MatrixIdentity(modelMatrix);
    modelMatrix[2][3] = offset;
    vtkm::Matrix<vtkm::Float32, 4, 4> worldToCamera =
      vtkm::MatrixMultiply(modelMatrix, Camera.CreateViewMatrix());
    vtkm::Matrix<vtkm::Float32, 4, 4> WorldToProjection = vtkm::MatrixMultiply(
      Camera.CreateProjectionMatrix(Canvas->GetWidth(), Canvas->GetHeight()), worldToCamera);

    vtkm::Id width = static_cast<vtkm::Id>(Canvas->GetWidth());
    vtkm::Id height = static_cast<vtkm::Id>(Canvas->GetHeight());
    vtkm::Id pixelCount = width * height;
    FrameBuffer.PrepareForOutput(pixelCount, DeviceTag());

    if (ShowInternalZones && !IsOverlay)
    {
      vtkm::cont::ArrayHandleConstant<vtkm::Int64> clear(ClearValue, pixelCount);
      vtkm::cont::Algorithm::Copy(clear, FrameBuffer);
    }
    else
    {
      VTKM_ASSERT(SolidDepthBuffer.GetNumberOfValues() == pixelCount);
      CopyIntoFrameBuffer bufferCopy;
      vtkm::worklet::DispatcherMapField<CopyIntoFrameBuffer>(bufferCopy)
        .Invoke(Canvas->GetColorBuffer(), SolidDepthBuffer, FrameBuffer);
    }
    //
    // detect a 2D camera and set the correct viewport.
    // The View port specifies what the region of the screen
    // to draw to which baiscally modifies the width and the
    // height of the "canvas"
    //
    vtkm::Id xOffset = 0;
    vtkm::Id yOffset = 0;
    vtkm::Id subsetWidth = width;
    vtkm::Id subsetHeight = height;

    bool ortho2d = Camera.GetMode() == vtkm::rendering::Camera::MODE_2D;
    if (ortho2d)
    {
      vtkm::Float32 vl, vr, vb, vt;
      Camera.GetRealViewport(width, height, vl, vr, vb, vt);
      vtkm::Float32 _x = static_cast<vtkm::Float32>(width) * (1.f + vl) / 2.f;
      vtkm::Float32 _y = static_cast<vtkm::Float32>(height) * (1.f + vb) / 2.f;
      vtkm::Float32 _w = static_cast<vtkm::Float32>(width) * (vr - vl) / 2.f;
      vtkm::Float32 _h = static_cast<vtkm::Float32>(height) * (vt - vb) / 2.f;

      subsetWidth = static_cast<vtkm::Id>(_w);
      subsetHeight = static_cast<vtkm::Id>(_h);
      yOffset = static_cast<vtkm::Id>(_y);
      xOffset = static_cast<vtkm::Id>(_x);
    }

    const bool isSupportedField = ScalarField.IsFieldCell() || ScalarField.IsFieldPoint();
    if (!isSupportedField)
    {
      throw vtkm::cont::ErrorBadValue("Field not associated with cell set or points");
    }
    const bool isAssocPoints = ScalarField.IsFieldPoint();

    EdgePlotter<DeviceTag> plotter(WorldToProjection,
                                   width,
                                   height,
                                   subsetWidth,
                                   subsetHeight,
                                   xOffset,
                                   yOffset,
                                   isAssocPoints,
                                   ScalarFieldRange,
                                   ColorMap,
                                   FrameBuffer,
                                   Camera.GetClippingRange());
    vtkm::worklet::DispatcherMapField<EdgePlotter<DeviceTag>> plotterDispatcher(plotter);
    plotterDispatcher.SetDevice(DeviceTag());
    plotterDispatcher.Invoke(
      PointIndices, Coordinates, ScalarField.GetData().ResetTypes(vtkm::TypeListFieldScalar()));

    BufferConverter converter;
    vtkm::worklet::DispatcherMapField<BufferConverter> converterDispatcher(converter);
    converterDispatcher.SetDevice(DeviceTag());
    converterDispatcher.Invoke(FrameBuffer, Canvas->GetDepthBuffer(), Canvas->GetColorBuffer());
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
  bool IsOverlay;
  ColorMapHandle ColorMap;
  vtkm::cont::CoordinateSystem Coordinates;
  IndicesHandle PointIndices;
  vtkm::cont::Field ScalarField;
  vtkm::Range ScalarFieldRange;
  vtkm::cont::ArrayHandle<vtkm::Float32> SolidDepthBuffer;
  PackedFrameBufferHandle FrameBuffer;
}; // class Wireframer
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_Wireframer_h
