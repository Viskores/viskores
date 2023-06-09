//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/raytracing/VolumeRendererStructured.h>

#include <cmath>
#include <iostream>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using CartesianArrayHandle =
  vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

namespace
{

template <typename Device, typename Derived>
class LocatorAdapterBase
{
private:
public:
  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec3f_32& point) const
  {
    return static_cast<const Derived*>(this)->Locator.IsInside(point);
  }

  // Assumes point inside the data set
  VTKM_EXEC
  inline void LocateCell(vtkm::Id3& cell,
                         const vtkm::Vec3f_32& point,
                         vtkm::Vec3f_32& invSpacing,
                         vtkm::Vec3f& parametric) const
  {
    vtkm::Id cellId{};
    auto self = static_cast<const Derived*>(this);
    self->Locator.FindCell(point, cellId, parametric);
    cell = self->Conn.FlatToLogicalVisitIndex(cellId);
    self->ComputeInvSpacing(cell, point, invSpacing, parametric);
  }

  VTKM_EXEC
  inline void GetCellIndices(const vtkm::Id3& cell, vtkm::Vec<vtkm::Id, 8>& cellIndices) const
  {
    cellIndices = static_cast<const Derived*>(this)->Conn.GetIndices(cell);
  }

  VTKM_EXEC
  inline vtkm::Id GetCellIndex(const vtkm::Id3& cell) const
  {
    return static_cast<const Derived*>(this)->Conn.LogicalToFlatVisitIndex(cell);
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec3f_32& point) const
  {
    BOUNDS_CHECK(static_cast<const Derived*>(this)->Coordinates, index);
    point = static_cast<const Derived*>(this)->Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Id3& cell, vtkm::Vec3f_32& point) const
  {
    const vtkm::Id pointIndex =
      static_cast<const Derived*>(this)->Conn.LogicalToFlatIncidentIndex(cell);
    point = static_cast<const Derived*>(this)->Coordinates.Get(pointIndex);
  }
};

template <typename Device>
class RectilinearLocatorAdapter
  : public LocatorAdapterBase<Device, RectilinearLocatorAdapter<Device>>
{
private:
  friend LocatorAdapterBase<Device, RectilinearLocatorAdapter<Device>>;
  using DefaultConstHandle = typename DefaultHandle::ReadPortalType;
  using CartesianConstPortal = typename CartesianArrayHandle::ReadPortalType;

  CartesianConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    Conn;
  vtkm::exec::CellLocatorRectilinearGrid Locator;

  DefaultConstHandle CoordPortals[3];

  VTKM_EXEC
  inline void ComputeInvSpacing(vtkm::Id3& cell,
                                const vtkm::Vec3f_32&,
                                vtkm::Vec3f_32& invSpacing,
                                vtkm::Vec3f) const
  {
    vtkm::Vec3f p0{ CoordPortals[0].Get(cell[0]),
                    CoordPortals[1].Get(cell[1]),
                    CoordPortals[2].Get(cell[2]) };
    vtkm::Vec3f p1{ CoordPortals[0].Get(cell[0] + 1),
                    CoordPortals[1].Get(cell[1] + 1),
                    CoordPortals[2].Get(cell[2] + 1) };
    invSpacing = 1.f / (p1 - p0);
  }

public:
  RectilinearLocatorAdapter(const CartesianArrayHandle& coordinates,
                            vtkm::cont::CellSetStructured<3>& cellset,
                            vtkm::cont::CellLocatorRectilinearGrid& locator,
                            vtkm::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagCell(),
                                   vtkm::TopologyElementTagPoint(),
                                   token))
    , Locator((locator.PrepareForExecution(Device(), token)))
  {
    CoordPortals[0] = Coordinates.GetFirstPortal();
    CoordPortals[1] = Coordinates.GetSecondPortal();
    CoordPortals[2] = Coordinates.GetThirdPortal();
  }
}; // class RectilinearLocatorAdapter

template <typename Device>
class UniformLocatorAdapter : public LocatorAdapterBase<Device, UniformLocatorAdapter<Device>>
{
private:
  friend LocatorAdapterBase<Device, UniformLocatorAdapter<Device>>;
  using UniformArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using UniformConstPortal = typename UniformArrayHandle::ReadPortalType;

  UniformConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    Conn;
  vtkm::exec::CellLocatorUniformGrid Locator;

  vtkm::Vec3f_32 InvSpacing{ 0, 0, 0 };

  VTKM_EXEC
  inline void ComputeInvSpacing(vtkm::Id3&,
                                const vtkm::Vec3f_32&,
                                vtkm::Vec3f_32& invSpacing,
                                vtkm::Vec3f&) const
  {
    invSpacing = InvSpacing;
  }

public:
  UniformLocatorAdapter(const UniformArrayHandle& coordinates,
                        vtkm::cont::CellSetStructured<3>& cellset,
                        vtkm::cont::CellLocatorUniformGrid& locator,
                        vtkm::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagCell(),
                                   vtkm::TopologyElementTagPoint(),
                                   token))
    , Locator(locator.PrepareForExecution(Device(), token))
  {
    vtkm::Vec3f_32 spacing = Coordinates.GetSpacing();
    InvSpacing[0] = 1.f / spacing[0];
    InvSpacing[1] = 1.f / spacing[1];
    InvSpacing[2] = 1.f / spacing[2];
  }
}; // class UniformLocatorAdapter

} //namespace


template <typename DeviceAdapterTag, typename LocatorType>
class Sampler : public vtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ReadPortalType;
  ColorArrayPortal ColorMap;
  vtkm::Id ColorMapSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 SampleDistance;
  vtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;
  vtkm::Float32 MeshEpsilon;

public:
  VTKM_CONT
  Sampler(const ColorArrayHandle& colorMap,
          const vtkm::Float32& minScalar,
          const vtkm::Float32& maxScalar,
          const vtkm::Float32& sampleDistance,
          const LocatorType& locator,
          const vtkm::Float32& meshEpsilon,
          vtkm::cont::Token& token)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag(), token))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
    , MeshEpsilon(meshEpsilon)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  VTKM_EXEC void operator()(const vtkm::Vec3f_32& rayDir,
                            const vtkm::Vec3f_32& rayOrigin,
                            const vtkm::Float32& minDistance,
                            const vtkm::Float32& maxDistance,
                            ColorBufferType& colorBuffer,
                            ScalarPortalType& scalars,
                            const vtkm::Id& pixelIndex) const
  {
    vtkm::Vec4f_32 color;
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
    {
      return; //TODO: Compact? or just image subset...
    }

    //get the initial sample position;
    vtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    vtkm::Float32 distance = minDistance + MeshEpsilon;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }

    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    bool newCell = true;
    vtkm::Vec3f parametric{ -1.f, -1.f, -1.f };
    vtkm::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);

    vtkm::Float32 scalar0 = 0.f;
    vtkm::Float32 scalar1minus0 = 0.f;
    vtkm::Float32 scalar2minus3 = 0.f;
    vtkm::Float32 scalar3 = 0.f;
    vtkm::Float32 scalar4 = 0.f;
    vtkm::Float32 scalar5minus4 = 0.f;
    vtkm::Float32 scalar6minus7 = 0.f;
    vtkm::Float32 scalar7 = 0.f;

    vtkm::Id3 cell(0, 0, 0);
    vtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);

    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      vtkm::Float32 mint = vtkm::Min(parametric[0], vtkm::Min(parametric[1], parametric[2]));
      vtkm::Float32 maxt = vtkm::Max(parametric[0], vtkm::Max(parametric[1], parametric[2]));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        vtkm::Vec<vtkm::Id, 8> cellIndices;
        Locator.LocateCell(cell, sampleLocation, invSpacing, parametric);
        Locator.GetCellIndices(cell, cellIndices);
        Locator.GetPoint(cellIndices[0], bottomLeft);

        scalar0 = vtkm::Float32(scalars.Get(cellIndices[0]));
        auto scalar1 = vtkm::Float32(scalars.Get(cellIndices[1]));
        auto scalar2 = vtkm::Float32(scalars.Get(cellIndices[2]));
        scalar3 = vtkm::Float32(scalars.Get(cellIndices[3]));
        scalar4 = vtkm::Float32(scalars.Get(cellIndices[4]));
        auto scalar5 = vtkm::Float32(scalars.Get(cellIndices[5]));
        auto scalar6 = vtkm::Float32(scalars.Get(cellIndices[6]));
        scalar7 = vtkm::Float32(scalars.Get(cellIndices[7]));

        // save ourselves a couple extra instructions
        scalar6minus7 = scalar6 - scalar7;
        scalar5minus4 = scalar5 - scalar4;
        scalar1minus0 = scalar1 - scalar0;
        scalar2minus3 = scalar2 - scalar3;

        newCell = false;
      }

      vtkm::Float32 lerped76 = scalar7 + parametric[0] * scalar6minus7;
      vtkm::Float32 lerped45 = scalar4 + parametric[0] * scalar5minus4;
      vtkm::Float32 lerpedTop = lerped45 + parametric[1] * (lerped76 - lerped45);

      vtkm::Float32 lerped01 = scalar0 + parametric[0] * scalar1minus0;
      vtkm::Float32 lerped32 = scalar3 + parametric[0] * scalar2minus3;
      vtkm::Float32 lerpedBottom = lerped01 + parametric[1] * (lerped32 - lerped01);

      vtkm::Float32 finalScalar = lerpedBottom + parametric[2] * (lerpedTop - lerpedBottom);

      //normalize scalar
      finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

      auto colorIndex =
        static_cast<vtkm::Id>(finalScalar * static_cast<vtkm::Float32>(ColorMapSize));
      if (colorIndex < 0)
        colorIndex = 0;
      if (colorIndex > ColorMapSize)
        colorIndex = ColorMapSize;
      vtkm::Vec4f_32 sampleColor = ColorMap.Get(colorIndex);

      //composite
      vtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      // terminate the ray early if it became completely opaque.
      if (color[3] >= 1.f)
        break;

      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      parametric = (sampleLocation - bottomLeft) * invSpacing;
    }

    color[0] = vtkm::Min(color[0], 1.f);
    color[1] = vtkm::Min(color[1], 1.f);
    color[2] = vtkm::Min(color[2], 1.f);
    color[3] = vtkm::Min(color[3], 1.f);

    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //Sampler

template <typename DeviceAdapterTag, typename LocatorType>
class SamplerCellAssoc : public vtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ReadPortalType;
  ColorArrayPortal ColorMap;
  vtkm::Id ColorMapSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 SampleDistance;
  vtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;
  vtkm::Float32 MeshEpsilon;

public:
  VTKM_CONT
  SamplerCellAssoc(const ColorArrayHandle& colorMap,
                   const vtkm::Float32& minScalar,
                   const vtkm::Float32& maxScalar,
                   const vtkm::Float32& sampleDistance,
                   const LocatorType& locator,
                   const vtkm::Float32& meshEpsilon,
                   vtkm::cont::Token& token)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag(), token))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
    , MeshEpsilon(meshEpsilon)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  VTKM_EXEC void operator()(const vtkm::Vec3f_32& rayDir,
                            const vtkm::Vec3f_32& rayOrigin,
                            const vtkm::Float32& minDistance,
                            const vtkm::Float32& maxDistance,
                            ColorBufferType& colorBuffer,
                            const ScalarPortalType& scalars,
                            const vtkm::Id& pixelIndex) const
  {
    vtkm::Vec4f_32 color;
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
    {
      return; //TODO: Compact? or just image subset...
    }

    //get the initial sample position;
    vtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    vtkm::Float32 distance = minDistance + MeshEpsilon;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }

    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    bool newCell = true;
    vtkm::Vec3f parametric{ -1.f, -1.f, -1.f };
    vtkm::Float32 scalar0 = 0.f;
    vtkm::Vec4f_32 sampleColor(0.f, 0.f, 0.f, 0.f);
    vtkm::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);

    vtkm::Id3 cell(0, 0, 0);
    vtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);

    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      vtkm::Float32 mint = vtkm::Min(parametric[0], vtkm::Min(parametric[1], parametric[2]));
      vtkm::Float32 maxt = vtkm::Max(parametric[0], vtkm::Max(parametric[1], parametric[2]));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        Locator.LocateCell(cell, sampleLocation, invSpacing, parametric);
        vtkm::Id cellId = Locator.GetCellIndex(cell);
        Locator.GetMinPoint(cell, bottomLeft);

        scalar0 = vtkm::Float32(scalars.Get(cellId));
        vtkm::Float32 normalizedScalar = (scalar0 - MinScalar) * InverseDeltaScalar;

        auto colorIndex =
          static_cast<vtkm::Id>(normalizedScalar * static_cast<vtkm::Float32>(ColorMapSize));
        if (colorIndex < 0)
          colorIndex = 0;
        if (colorIndex > ColorMapSize)
          colorIndex = ColorMapSize;
        sampleColor = ColorMap.Get(colorIndex);

        newCell = false;
      }

      // just repeatably composite
      vtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      // terminate the ray early if it became completely opaque.
      if (color[3] >= 1.f)
        break;

      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      parametric = (sampleLocation - bottomLeft) * invSpacing;
    }

    color[0] = vtkm::Min(color[0], 1.f);
    color[1] = vtkm::Min(color[1], 1.f);
    color[2] = vtkm::Min(color[2], 1.f);
    color[3] = vtkm::Min(color[3], 1.f);

    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //SamplerCell

class CalcRayStart : public vtkm::worklet::WorkletMapField
{
  vtkm::Float32 Xmin;
  vtkm::Float32 Ymin;
  vtkm::Float32 Zmin;
  vtkm::Float32 Xmax;
  vtkm::Float32 Ymax;
  vtkm::Float32 Zmax;

public:
  VTKM_CONT
  explicit CalcRayStart(const vtkm::Bounds boundingBox)
  {
    Xmin = static_cast<vtkm::Float32>(boundingBox.X.Min);
    Xmax = static_cast<vtkm::Float32>(boundingBox.X.Max);
    Ymin = static_cast<vtkm::Float32>(boundingBox.Y.Min);
    Ymax = static_cast<vtkm::Float32>(boundingBox.Y.Max);
    Zmin = static_cast<vtkm::Float32>(boundingBox.Z.Min);
    Zmax = static_cast<vtkm::Float32>(boundingBox.Z.Max);
  }

  VTKM_EXEC
  static vtkm::Float32 rcp(vtkm::Float32 f) { return 1.0f / f; }

  VTKM_EXEC
  static vtkm::Float32 rcp_safe(vtkm::Float32 f) { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }

  using ControlSignature = void(FieldIn, FieldOut, FieldInOut, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  template <typename Precision>
  VTKM_EXEC void operator()(const vtkm::Vec<Precision, 3>& rayDir,
                            vtkm::Float32& minDistance,
                            vtkm::Float32& distance,
                            vtkm::Float32& maxDistance,
                            const vtkm::Vec<Precision, 3>& rayOrigin) const
  {
    auto dirx = static_cast<vtkm::Float32>(rayDir[0]);
    auto diry = static_cast<vtkm::Float32>(rayDir[1]);
    auto dirz = static_cast<vtkm::Float32>(rayDir[2]);
    auto origx = static_cast<vtkm::Float32>(rayOrigin[0]);
    auto origy = static_cast<vtkm::Float32>(rayOrigin[1]);
    auto origz = static_cast<vtkm::Float32>(rayOrigin[2]);

    vtkm::Float32 invDirx = rcp_safe(dirx);
    vtkm::Float32 invDiry = rcp_safe(diry);
    vtkm::Float32 invDirz = rcp_safe(dirz);

    vtkm::Float32 odirx = origx * invDirx;
    vtkm::Float32 odiry = origy * invDiry;
    vtkm::Float32 odirz = origz * invDirz;

    vtkm::Float32 xmin = Xmin * invDirx - odirx;
    vtkm::Float32 ymin = Ymin * invDiry - odiry;
    vtkm::Float32 zmin = Zmin * invDirz - odirz;
    vtkm::Float32 xmax = Xmax * invDirx - odirx;
    vtkm::Float32 ymax = Ymax * invDiry - odiry;
    vtkm::Float32 zmax = Zmax * invDirz - odirz;


    minDistance = vtkm::Max(
      vtkm::Max(vtkm::Max(vtkm::Min(ymin, ymax), vtkm::Min(xmin, xmax)), vtkm::Min(zmin, zmax)),
      minDistance);
    vtkm::Float32 exitDistance =
      vtkm::Min(vtkm::Min(vtkm::Max(ymin, ymax), vtkm::Max(xmin, xmax)), vtkm::Max(zmin, zmax));
    maxDistance = vtkm::Min(maxDistance, exitDistance);
    if (maxDistance < minDistance)
    {
      minDistance = -1.f; //flag for miss
    }
    else
    {
      distance = minDistance;
    }
  }
}; //class CalcRayStart

void VolumeRendererStructured::SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void VolumeRendererStructured::SetData(const vtkm::cont::CoordinateSystem& coords,
                                       const vtkm::cont::Field& scalarField,
                                       const vtkm::cont::CellSetStructured<3>& cellset,
                                       const vtkm::Range& scalarRange)
{
  IsUniformDataSet = !coords.GetData().IsType<CartesianArrayHandle>();
  IsSceneDirty = true;
  SpatialExtent = coords.GetBounds();
  Coordinates = coords;
  ScalarField = &scalarField;
  Cellset = cellset;
  ScalarRange = scalarRange;
}

void VolumeRendererStructured::Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
{
  auto functor = [&](auto device) {
    using Device = typename std::decay_t<decltype(device)>;
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->RenderOnDevice(rays, device);
    return true;
  };
  vtkm::cont::TryExecute(functor);
}

//void
//VolumeRendererStructured::Render(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays)
//{
//  RenderFunctor<vtkm::Float64> functor(this, rays);
//  vtkm::cont::TryExecute(functor);
//}

template <typename Precision, typename Device>
void VolumeRendererStructured::RenderOnDevice(vtkm::rendering::raytracing::Ray<Precision>& rays,
                                              Device)
{
  vtkm::cont::Timer renderTimer{ Device() };
  renderTimer.Start();

  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("volume_render_structured");
  logger->AddLogData("device", GetDeviceString(Device()));

  vtkm::Vec3f_32 extent;
  extent[0] = static_cast<vtkm::Float32>(this->SpatialExtent.X.Length());
  extent[1] = static_cast<vtkm::Float32>(this->SpatialExtent.Y.Length());
  extent[2] = static_cast<vtkm::Float32>(this->SpatialExtent.Z.Length());
  vtkm::Float32 mag_extent = vtkm::Magnitude(extent);
  vtkm::Float32 meshEpsilon = mag_extent * 0.0001f;
  if (SampleDistance <= 0.f)
  {
    const vtkm::Float32 defaultNumberOfSamples = 200.f;
    SampleDistance = mag_extent / defaultNumberOfSamples;
  }

  vtkm::cont::Invoker invoke;

  vtkm::cont::Timer timer{ Device() };
  timer.Start();
  invoke(CalcRayStart{ this->SpatialExtent },
         rays.Dir,
         rays.MinDistance,
         rays.Distance,
         rays.MaxDistance,
         rays.Origin);
  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("calc_ray_start", time);

  timer.Start();

  const bool isSupportedField = ScalarField->IsCellField() || ScalarField->IsPointField();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  const bool isAssocPoints = ScalarField->IsPointField();

  if (IsUniformDataSet)
  {
    vtkm::cont::Token token;
    vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
    vertices =
      Coordinates.GetData().AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    vtkm::cont::CellLocatorUniformGrid uniLocator;
    uniLocator.SetCellSet(this->Cellset);
    uniLocator.SetCoordinates(this->Coordinates);
    UniformLocatorAdapter<Device> locator(vertices, this->Cellset, uniLocator, token);

    if (isAssocPoints)
    {
      auto sampler = Sampler<Device, UniformLocatorAdapter<Device>>(ColorMap,
                                                                    vtkm::Float32(ScalarRange.Min),
                                                                    vtkm::Float32(ScalarRange.Max),
                                                                    SampleDistance,
                                                                    locator,
                                                                    meshEpsilon,
                                                                    token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
    else
    {
      auto sampler =
        SamplerCellAssoc<Device, UniformLocatorAdapter<Device>>(ColorMap,
                                                                vtkm::Float32(ScalarRange.Min),
                                                                vtkm::Float32(ScalarRange.Max),
                                                                SampleDistance,
                                                                locator,
                                                                meshEpsilon,
                                                                token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
  }
  else
  {
    vtkm::cont::Token token;
    CartesianArrayHandle vertices;
    vertices = Coordinates.GetData().AsArrayHandle<CartesianArrayHandle>();
    vtkm::cont::CellLocatorRectilinearGrid rectLocator;
    rectLocator.SetCellSet(this->Cellset);
    rectLocator.SetCoordinates(this->Coordinates);
    RectilinearLocatorAdapter<Device> locator(vertices, Cellset, rectLocator, token);

    if (isAssocPoints)
    {
      auto sampler =
        Sampler<Device, RectilinearLocatorAdapter<Device>>(ColorMap,
                                                           vtkm::Float32(ScalarRange.Min),
                                                           vtkm::Float32(ScalarRange.Max),
                                                           SampleDistance,
                                                           locator,
                                                           meshEpsilon,
                                                           token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
    else
    {
      auto sampler =
        SamplerCellAssoc<Device, RectilinearLocatorAdapter<Device>>(ColorMap,
                                                                    vtkm::Float32(ScalarRange.Min),
                                                                    vtkm::Float32(ScalarRange.Max),
                                                                    SampleDistance,
                                                                    locator,
                                                                    meshEpsilon,
                                                                    token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
  }

  time = timer.GetElapsedTime();
  logger->AddLogData("sample", time);

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} //Render

void VolumeRendererStructured::SetSampleDistance(const vtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}
}
}
} //namespace vtkm::rendering::raytracing
