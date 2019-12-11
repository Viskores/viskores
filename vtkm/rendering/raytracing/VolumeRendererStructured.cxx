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

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace
{

template <typename Device>
class RectilinearLocator
{
protected:
  using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using CartesianArrayHandle =
    vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;
  using DefaultConstHandle = typename DefaultHandle::ExecutionTypes<Device>::PortalConst;
  using CartesianConstPortal = typename CartesianArrayHandle::ExecutionTypes<Device>::PortalConst;

  DefaultConstHandle CoordPortals[3];
  CartesianConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    Conn;
  vtkm::Id3 PointDimensions;
  vtkm::Vec3f_32 MinPoint;
  vtkm::Vec3f_32 MaxPoint;

public:
  RectilinearLocator(const CartesianArrayHandle& coordinates,
                     vtkm::cont::CellSetStructured<3>& cellset)
    : Coordinates(coordinates.PrepareForInput(Device()))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagCell(),
                                   vtkm::TopologyElementTagPoint()))
  {
    CoordPortals[0] = Coordinates.GetFirstPortal();
    CoordPortals[1] = Coordinates.GetSecondPortal();
    CoordPortals[2] = Coordinates.GetThirdPortal();
    PointDimensions = Conn.GetPointDimensions();
    MinPoint[0] =
      static_cast<vtkm::Float32>(coordinates.GetPortalConstControl().GetFirstPortal().Get(0));
    MinPoint[1] =
      static_cast<vtkm::Float32>(coordinates.GetPortalConstControl().GetSecondPortal().Get(0));
    MinPoint[2] =
      static_cast<vtkm::Float32>(coordinates.GetPortalConstControl().GetThirdPortal().Get(0));

    MaxPoint[0] = static_cast<vtkm::Float32>(
      coordinates.GetPortalConstControl().GetFirstPortal().Get(PointDimensions[0] - 1));
    MaxPoint[1] = static_cast<vtkm::Float32>(
      coordinates.GetPortalConstControl().GetSecondPortal().Get(PointDimensions[1] - 1));
    MaxPoint[2] = static_cast<vtkm::Float32>(
      coordinates.GetPortalConstControl().GetThirdPortal().Get(PointDimensions[2] - 1));
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec3f_32& point) const
  {
    bool inside = true;
    if (point[0] < MinPoint[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < MinPoint[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < MinPoint[2] || point[2] > MaxPoint[2])
      inside = false;
    return inside;
  }

  VTKM_EXEC
  inline void GetCellIndices(const vtkm::Id3& cell, vtkm::Vec<vtkm::Id, 8>& cellIndices) const
  {
    cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDimensions[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDimensions[0] * PointDimensions[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDimensions[0];
    cellIndices[7] = cellIndices[6] - 1;
  } // GetCellIndices

  //
  // Assumes point inside the data set
  //
  VTKM_EXEC
  inline void LocateCell(vtkm::Id3& cell,
                         const vtkm::Vec3f_32& point,
                         vtkm::Vec3f_32& invSpacing) const
  {
    for (vtkm::Int32 dim = 0; dim < 3; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        cell[dim] = PointDimensions[dim] - 2;
        continue;
      }

      bool found = false;
      vtkm::Float32 minVal = static_cast<vtkm::Float32>(CoordPortals[dim].Get(cell[dim]));
      const vtkm::Id searchDir = (point[dim] - minVal >= 0.f) ? 1 : -1;
      vtkm::Float32 maxVal = static_cast<vtkm::Float32>(CoordPortals[dim].Get(cell[dim] + 1));

      while (!found)
      {
        if (point[dim] >= minVal && point[dim] < maxVal)
        {
          found = true;
          continue;
        }

        cell[dim] += searchDir;
        vtkm::Id nextCellId = searchDir == 1 ? cell[dim] + 1 : cell[dim];
        BOUNDS_CHECK(CoordPortals[dim], nextCellId);
        vtkm::Float32 next = static_cast<vtkm::Float32>(CoordPortals[dim].Get(nextCellId));
        if (searchDir == 1)
        {
          minVal = maxVal;
          maxVal = next;
        }
        else
        {
          maxVal = minVal;
          minVal = next;
        }
      }
      invSpacing[dim] = 1.f / (maxVal - minVal);
    }
  } // LocateCell

  VTKM_EXEC
  inline vtkm::Id GetCellIndex(const vtkm::Id3& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec3f_32& point) const
  {
    BOUNDS_CHECK(Coordinates, index);
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Id3& cell, vtkm::Vec3f_32& point) const
  {
    const vtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }
}; // class RectilinearLocator

template <typename Device>
class UniformLocator
{
protected:
  using UniformArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using UniformConstPortal = typename UniformArrayHandle::ExecutionTypes<Device>::PortalConst;

  vtkm::Id3 PointDimensions;
  vtkm::Vec3f_32 Origin;
  vtkm::Vec3f_32 InvSpacing;
  vtkm::Vec3f_32 MaxPoint;
  UniformConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    Conn;

public:
  UniformLocator(const UniformArrayHandle& coordinates, vtkm::cont::CellSetStructured<3>& cellset)
    : Coordinates(coordinates.PrepareForInput(Device()))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagCell(),
                                   vtkm::TopologyElementTagPoint()))
  {
    Origin = Coordinates.GetOrigin();
    PointDimensions = Conn.GetPointDimensions();
    vtkm::Vec3f_32 spacing = Coordinates.GetSpacing();

    vtkm::Vec3f_32 unitLength;
    unitLength[0] = static_cast<vtkm::Float32>(PointDimensions[0] - 1);
    unitLength[1] = static_cast<vtkm::Float32>(PointDimensions[1] - 1);
    unitLength[2] = static_cast<vtkm::Float32>(PointDimensions[2] - 1);
    MaxPoint = Origin + spacing * unitLength;
    InvSpacing[0] = 1.f / spacing[0];
    InvSpacing[1] = 1.f / spacing[1];
    InvSpacing[2] = 1.f / spacing[2];
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec3f_32& point) const
  {
    bool inside = true;
    if (point[0] < Origin[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < Origin[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < Origin[2] || point[2] > MaxPoint[2])
      inside = false;
    return inside;
  }

  VTKM_EXEC
  inline void GetCellIndices(const vtkm::Id3& cell, vtkm::Vec<vtkm::Id, 8>& cellIndices) const
  {
    cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDimensions[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDimensions[0] * PointDimensions[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDimensions[0];
    cellIndices[7] = cellIndices[6] - 1;
  } // GetCellIndices

  VTKM_EXEC
  inline vtkm::Id GetCellIndex(const vtkm::Id3& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  VTKM_EXEC
  inline void LocateCell(vtkm::Id3& cell,
                         const vtkm::Vec3f_32& point,
                         vtkm::Vec3f_32& invSpacing) const
  {
    vtkm::Vec3f_32 temp = point;
    temp = temp - Origin;
    temp = temp * InvSpacing;
    //make sure that if we border the upper edge, we sample the correct cell
    if (temp[0] == vtkm::Float32(PointDimensions[0] - 1))
      temp[0] = vtkm::Float32(PointDimensions[0] - 2);
    if (temp[1] == vtkm::Float32(PointDimensions[1] - 1))
      temp[1] = vtkm::Float32(PointDimensions[1] - 2);
    if (temp[2] == vtkm::Float32(PointDimensions[2] - 1))
      temp[2] = vtkm::Float32(PointDimensions[2] - 2);
    cell = temp;
    invSpacing = InvSpacing;
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec3f_32& point) const
  {
    BOUNDS_CHECK(Coordinates, index);
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Id3& cell, vtkm::Vec3f_32& point) const
  {
    const vtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }

}; // class UniformLocator


} //namespace


template <typename DeviceAdapterTag, typename LocatorType>
class Sampler : public vtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ExecutionTypes<DeviceAdapterTag>::PortalConst;
  ColorArrayPortal ColorMap;
  vtkm::Id ColorMapSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 SampleDistance;
  vtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;

public:
  VTKM_CONT
  Sampler(const ColorArrayHandle& colorMap,
          const vtkm::Float32& minScalar,
          const vtkm::Float32& maxScalar,
          const vtkm::Float32& sampleDistance,
          const LocatorType& locator)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag()))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
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
    vtkm::Float32 distance = minDistance + 0.0001f;
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
    vtkm::Vec3f_32 bottomLeft(0, 0, 0);
    bool newCell = true;
    //check to see if we left the cell
    vtkm::Float32 tx = 0.f;
    vtkm::Float32 ty = 0.f;
    vtkm::Float32 tz = 0.f;
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
      vtkm::Float32 mint = vtkm::Min(tx, vtkm::Min(ty, tz));
      vtkm::Float32 maxt = vtkm::Max(tx, vtkm::Max(ty, tz));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;

      if (newCell)
      {

        vtkm::Vec<vtkm::Id, 8> cellIndices;
        Locator.LocateCell(cell, sampleLocation, invSpacing);
        Locator.GetCellIndices(cell, cellIndices);
        Locator.GetPoint(cellIndices[0], bottomLeft);

        scalar0 = vtkm::Float32(scalars.Get(cellIndices[0]));
        vtkm::Float32 scalar1 = vtkm::Float32(scalars.Get(cellIndices[1]));
        vtkm::Float32 scalar2 = vtkm::Float32(scalars.Get(cellIndices[2]));
        scalar3 = vtkm::Float32(scalars.Get(cellIndices[3]));
        scalar4 = vtkm::Float32(scalars.Get(cellIndices[4]));
        vtkm::Float32 scalar5 = vtkm::Float32(scalars.Get(cellIndices[5]));
        vtkm::Float32 scalar6 = vtkm::Float32(scalars.Get(cellIndices[6]));
        scalar7 = vtkm::Float32(scalars.Get(cellIndices[7]));

        // save ourselves a couple extra instructions
        scalar6minus7 = scalar6 - scalar7;
        scalar5minus4 = scalar5 - scalar4;
        scalar1minus0 = scalar1 - scalar0;
        scalar2minus3 = scalar2 - scalar3;

        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

        newCell = false;
      }

      vtkm::Float32 lerped76 = scalar7 + tx * scalar6minus7;
      vtkm::Float32 lerped45 = scalar4 + tx * scalar5minus4;
      vtkm::Float32 lerpedTop = lerped45 + ty * (lerped76 - lerped45);

      vtkm::Float32 lerped01 = scalar0 + tx * scalar1minus0;
      vtkm::Float32 lerped32 = scalar3 + tx * scalar2minus3;
      vtkm::Float32 lerpedBottom = lerped01 + ty * (lerped32 - lerped01);

      vtkm::Float32 finalScalar = lerpedBottom + tz * (lerpedTop - lerpedBottom);
      //normalize scalar
      finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

      vtkm::Id colorIndex =
        static_cast<vtkm::Id>(finalScalar * static_cast<vtkm::Float32>(ColorMapSize));
      if (colorIndex < 0)
        colorIndex = 0;
      if (colorIndex > ColorMapSize)
        colorIndex = ColorMapSize;

      vtkm::Vec4f_32 sampleColor = ColorMap.Get(colorIndex);

      //composite
      sampleColor[3] *= (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];
      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      //this is linear could just do an addition
      tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
      ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
      tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

      if (color[3] >= 1.f)
        break;
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
  using ColorArrayPortal = typename ColorArrayHandle::ExecutionTypes<DeviceAdapterTag>::PortalConst;
  ColorArrayPortal ColorMap;
  vtkm::Id ColorMapSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 SampleDistance;
  vtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;

public:
  VTKM_CONT
  SamplerCellAssoc(const ColorArrayHandle& colorMap,
                   const vtkm::Float32& minScalar,
                   const vtkm::Float32& maxScalar,
                   const vtkm::Float32& sampleDistance,
                   const LocatorType& locator)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag()))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
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
      return; //TODO: Compact? or just image subset...
    //get the initial sample position;
    vtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    vtkm::Float32 distance = minDistance + 0.0001f;
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
    vtkm::Float32 tx = 2.f;
    vtkm::Float32 ty = 2.f;
    vtkm::Float32 tz = 2.f;
    vtkm::Float32 scalar0 = 0.f;
    vtkm::Vec4f_32 sampleColor(0.f, 0.f, 0.f, 0.f);
    vtkm::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);
    vtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);
    vtkm::Id3 cell(0, 0, 0);
    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      vtkm::Float32 mint = vtkm::Min(tx, vtkm::Min(ty, tz));
      vtkm::Float32 maxt = vtkm::Max(tx, vtkm::Max(ty, tz));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        Locator.LocateCell(cell, sampleLocation, invSpacing);
        vtkm::Id cellId = Locator.GetCellIndex(cell);

        scalar0 = vtkm::Float32(scalars.Get(cellId));
        vtkm::Float32 normalizedScalar = (scalar0 - MinScalar) * InverseDeltaScalar;
        vtkm::Id colorIndex =
          static_cast<vtkm::Id>(normalizedScalar * static_cast<vtkm::Float32>(ColorMapSize));
        if (colorIndex < 0)
          colorIndex = 0;
        if (colorIndex > ColorMapSize)
          colorIndex = ColorMapSize;
        sampleColor = ColorMap.Get(colorIndex);
        Locator.GetMinPoint(cell, bottomLeft);
        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];
        newCell = false;
      }

      // just repeatably composite
      vtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];
      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      if (color[3] >= 1.f)
        break;
      tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
      ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
      tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];
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
  CalcRayStart(const vtkm::Bounds boundingBox)
  {
    Xmin = static_cast<vtkm::Float32>(boundingBox.X.Min);
    Xmax = static_cast<vtkm::Float32>(boundingBox.X.Max);
    Ymin = static_cast<vtkm::Float32>(boundingBox.Y.Min);
    Ymax = static_cast<vtkm::Float32>(boundingBox.Y.Max);
    Zmin = static_cast<vtkm::Float32>(boundingBox.Z.Min);
    Zmax = static_cast<vtkm::Float32>(boundingBox.Z.Max);
  }

  VTKM_EXEC
  vtkm::Float32 rcp(vtkm::Float32 f) const { return 1.0f / f; }

  VTKM_EXEC
  vtkm::Float32 rcp_safe(vtkm::Float32 f) const { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }

  using ControlSignature = void(FieldIn, FieldOut, FieldInOut, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  template <typename Precision>
  VTKM_EXEC void operator()(const vtkm::Vec<Precision, 3>& rayDir,
                            vtkm::Float32& minDistance,
                            vtkm::Float32& distance,
                            vtkm::Float32& maxDistance,
                            const vtkm::Vec<Precision, 3>& rayOrigin) const
  {
    vtkm::Float32 dirx = static_cast<vtkm::Float32>(rayDir[0]);
    vtkm::Float32 diry = static_cast<vtkm::Float32>(rayDir[1]);
    vtkm::Float32 dirz = static_cast<vtkm::Float32>(rayDir[2]);
    vtkm::Float32 origx = static_cast<vtkm::Float32>(rayOrigin[0]);
    vtkm::Float32 origy = static_cast<vtkm::Float32>(rayOrigin[1]);
    vtkm::Float32 origz = static_cast<vtkm::Float32>(rayOrigin[2]);

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

VolumeRendererStructured::VolumeRendererStructured()
{
  IsSceneDirty = false;
  IsUniformDataSet = true;
  SampleDistance = -1.f;
}

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
  Coordinates = coords.GetData();
  ScalarField = &scalarField;
  Cellset = cellset;
  ScalarRange = scalarRange;
}

template <typename Precision>
struct VolumeRendererStructured::RenderFunctor
{
protected:
  vtkm::rendering::raytracing::VolumeRendererStructured* Self;
  vtkm::rendering::raytracing::Ray<Precision>& Rays;

public:
  VTKM_CONT
  RenderFunctor(vtkm::rendering::raytracing::VolumeRendererStructured* self,
                vtkm::rendering::raytracing::Ray<Precision>& rays)
    : Self(self)
    , Rays(rays)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->Self->RenderOnDevice(this->Rays, Device());
    return true;
  }
};

void VolumeRendererStructured::Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
{
  RenderFunctor<vtkm::Float32> functor(this, rays);
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

  if (SampleDistance <= 0.f)
  {
    vtkm::Vec3f_32 extent;
    extent[0] = static_cast<vtkm::Float32>(this->SpatialExtent.X.Length());
    extent[1] = static_cast<vtkm::Float32>(this->SpatialExtent.Y.Length());
    extent[2] = static_cast<vtkm::Float32>(this->SpatialExtent.Z.Length());
    const vtkm::Float32 defaultNumberOfSamples = 200.f;
    SampleDistance = vtkm::Magnitude(extent) / defaultNumberOfSamples;
  }

  vtkm::cont::Timer timer{ Device() };
  timer.Start();
  vtkm::worklet::DispatcherMapField<CalcRayStart> calcRayStartDispatcher(
    CalcRayStart(this->SpatialExtent));
  calcRayStartDispatcher.SetDevice(Device());
  calcRayStartDispatcher.Invoke(
    rays.Dir, rays.MinDistance, rays.Distance, rays.MaxDistance, rays.Origin);

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("calc_ray_start", time);
  timer.Start();

  const bool isSupportedField = ScalarField->IsFieldCell() || ScalarField->IsFieldPoint();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  const bool isAssocPoints = ScalarField->IsFieldPoint();

  if (IsUniformDataSet)
  {
    vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
    vertices = Coordinates.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    UniformLocator<Device> locator(vertices, Cellset);

    if (isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField<Sampler<Device, UniformLocator<Device>>> samplerDispatcher(
        Sampler<Device, UniformLocator<Device>>(ColorMap,
                                                vtkm::Float32(ScalarRange.Min),
                                                vtkm::Float32(ScalarRange.Max),
                                                SampleDistance,
                                                locator));
      samplerDispatcher.SetDevice(Device());
      samplerDispatcher.Invoke(rays.Dir,
                               rays.Origin,
                               rays.MinDistance,
                               rays.MaxDistance,
                               rays.Buffers.at(0).Buffer,
                               ScalarField->GetData().ResetTypes(vtkm::TypeListFieldScalar()));
    }
    else
    {
      vtkm::worklet::DispatcherMapField<SamplerCellAssoc<Device, UniformLocator<Device>>>(
        SamplerCellAssoc<Device, UniformLocator<Device>>(ColorMap,
                                                         vtkm::Float32(ScalarRange.Min),
                                                         vtkm::Float32(ScalarRange.Max),
                                                         SampleDistance,
                                                         locator))
        .Invoke(rays.Dir,
                rays.Origin,
                rays.MinDistance,
                rays.MaxDistance,
                rays.Buffers.at(0).Buffer,
                ScalarField->GetData().ResetTypes(vtkm::TypeListFieldScalar()));
    }
  }
  else
  {
    CartesianArrayHandle vertices;
    vertices = Coordinates.Cast<CartesianArrayHandle>();
    RectilinearLocator<Device> locator(vertices, Cellset);
    if (isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField<Sampler<Device, RectilinearLocator<Device>>>
        samplerDispatcher(
          Sampler<Device, RectilinearLocator<Device>>(ColorMap,
                                                      vtkm::Float32(ScalarRange.Min),
                                                      vtkm::Float32(ScalarRange.Max),
                                                      SampleDistance,
                                                      locator));
      samplerDispatcher.SetDevice(Device());
      samplerDispatcher.Invoke(rays.Dir,
                               rays.Origin,
                               rays.MinDistance,
                               rays.MaxDistance,
                               rays.Buffers.at(0).Buffer,
                               ScalarField->GetData().ResetTypes(vtkm::TypeListFieldScalar()));
    }
    else
    {
      vtkm::worklet::DispatcherMapField<SamplerCellAssoc<Device, RectilinearLocator<Device>>>
        rectilinearLocatorDispatcher(
          SamplerCellAssoc<Device, RectilinearLocator<Device>>(ColorMap,
                                                               vtkm::Float32(ScalarRange.Min),
                                                               vtkm::Float32(ScalarRange.Max),
                                                               SampleDistance,
                                                               locator));
      rectilinearLocatorDispatcher.SetDevice(Device());
      rectilinearLocatorDispatcher.Invoke(
        rays.Dir,
        rays.Origin,
        rays.MinDistance,
        rays.MaxDistance,
        rays.Buffers.at(0).Buffer,
        ScalarField->GetData().ResetTypes(vtkm::TypeListFieldScalar()));
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
