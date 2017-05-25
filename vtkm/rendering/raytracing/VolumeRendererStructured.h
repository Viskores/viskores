//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
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
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_rendering_raytracing_VolumeRendererStructured_h
#define vtk_m_rendering_raytracing_VolumeRendererStructured_h

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/raytracing/Camera.h>
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
  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
  typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>
    CartesianArrayHandle;
  typedef typename DefaultHandle::ExecutionTypes<Device>::PortalConst DefaultConstHandle;
  typedef typename CartesianArrayHandle::ExecutionTypes<Device>::PortalConst CartesianConstPortal;

  vtkm::Float32 InverseDeltaScalar;
  DefaultConstHandle CoordPortals[3];
  CartesianConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, 3>
    Conn;
  vtkm::Id3 PointDimensions;
  vtkm::Vec<vtkm::Float32, 3> MinPoint;
  vtkm::Vec<vtkm::Float32, 3> MaxPoint;

public:
  RectilinearLocator(const CartesianArrayHandle& coordinates,
                     vtkm::cont::CellSetStructured<3>& cellset)
    : Coordinates(coordinates.PrepareForInput(Device()))
    , Conn(cellset.PrepareForInput(Device(), vtkm::TopologyElementTagPoint(),
                                   vtkm::TopologyElementTagCell()))
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
  inline bool IsInside(const vtkm::Vec<vtkm::Float32, 3>& point) const
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
  inline void GetCellIndices(const vtkm::Vec<vtkm::Id, 3>& cell,
                             vtkm::Vec<vtkm::Id, 8>& cellIndices) const
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
  inline void LocateCell(vtkm::Vec<vtkm::Id, 3>& cell, const vtkm::Vec<vtkm::Float32, 3>& point,
                         vtkm::Vec<vtkm::Float32, 3>& invSpacing) const
  {
    for (vtkm::Int32 dim = 0; dim < 3; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundry of the
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
  inline vtkm::Id GetCellIndex(const vtkm::Vec<vtkm::Id, 3>& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec<vtkm::Float32, 3>& point) const
  {
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Vec<vtkm::Id, 3>& cell,
                          vtkm::Vec<vtkm::Float32, 3>& point) const
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
  typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
  typedef typename UniformArrayHandle::ExecutionTypes<Device>::PortalConst UniformConstPortal;

  vtkm::Id3 PointDimensions;
  vtkm::Vec<vtkm::Float32, 3> Origin;
  vtkm::Vec<vtkm::Float32, 3> InvSpacing;
  vtkm::Vec<vtkm::Float32, 3> MaxPoint;
  UniformConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, 3>
    Conn;

public:
  UniformLocator(const UniformArrayHandle& coordinates, vtkm::cont::CellSetStructured<3>& cellset)
    : Coordinates(coordinates.PrepareForInput(Device()))
    , Conn(cellset.PrepareForInput(Device(), vtkm::TopologyElementTagPoint(),
                                   vtkm::TopologyElementTagCell()))
  {
    Origin = Coordinates.GetOrigin();
    PointDimensions = Conn.GetPointDimensions();
    vtkm::Vec<vtkm::Float32, 3> spacing = Coordinates.GetSpacing();

    vtkm::Vec<vtkm::Float32, 3> unitLength;
    unitLength[0] = static_cast<vtkm::Float32>(PointDimensions[0] - 1);
    unitLength[1] = static_cast<vtkm::Float32>(PointDimensions[1] - 1);
    unitLength[2] = static_cast<vtkm::Float32>(PointDimensions[2] - 1);
    MaxPoint = Origin + spacing * unitLength;
    InvSpacing[0] = 1.f / spacing[0];
    InvSpacing[1] = 1.f / spacing[1];
    InvSpacing[2] = 1.f / spacing[2];
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec<vtkm::Float32, 3>& point) const
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
  inline void GetCellIndices(const vtkm::Vec<vtkm::Id, 3>& cell,
                             vtkm::Vec<vtkm::Id, 8>& cellIndices) const
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
  inline vtkm::Id GetCellIndex(const vtkm::Vec<vtkm::Id, 3>& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  VTKM_EXEC
  inline void LocateCell(vtkm::Vec<vtkm::Id, 3>& cell, const vtkm::Vec<vtkm::Float32, 3>& point,
                         vtkm::Vec<vtkm::Float32, 3>& invSpacing) const
  {
    vtkm::Vec<vtkm::Float32, 3> temp = point;
    //make sure that if we border the upper edge, we sample the correct cell
    if (temp[0] == vtkm::Float32(PointDimensions[0] - 1))
      temp[0] = vtkm::Float32(PointDimensions[0] - 2);
    if (temp[1] == vtkm::Float32(PointDimensions[1] - 1))
      temp[0] = vtkm::Float32(PointDimensions[1] - 2);
    if (temp[2] == vtkm::Float32(PointDimensions[2] - 1))
      temp[0] = vtkm::Float32(PointDimensions[2] - 2);
    temp = temp - Origin;
    temp = temp * InvSpacing;
    cell = temp;
    invSpacing = InvSpacing;
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec<vtkm::Float32, 3>& point) const
  {
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Vec<vtkm::Id, 3>& cell,
                          vtkm::Vec<vtkm::Float32, 3>& point) const
  {
    const vtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }

}; // class UniformLocator

} //namespace
template <typename Device>
class VolumeRendererStructured
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
  typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>
    CartesianArrayHandle;

  template <typename LocatorType>
  class Sampler : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<Device>::PortalConst ColorArrayPortal;
    vtkm::Vec<vtkm::Float32, 3> CameraPosition;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    vtkm::Float32 MinScalar;
    vtkm::Float32 SampleDistance;
    vtkm::Float32 InverseDeltaScalar;
    LocatorType Locator;

  public:
    VTKM_CONT
    Sampler(vtkm::Vec<vtkm::Float32, 3> cameraPosition, const ColorArrayHandle& colorMap,
            const vtkm::Float32& minScalar, const vtkm::Float32& maxScalar,
            const vtkm::Float32& sampleDistance, const LocatorType& locator)
      : CameraPosition(cameraPosition)
      , ColorMap(colorMap.PrepareForInput(Device()))
      , MinScalar(minScalar)
      , SampleDistance(sampleDistance)
      , Locator(locator)
    {
      ColorMapSize = colorMap.GetNumberOfValues() - 1;
      if ((maxScalar - minScalar) != 0.f)
        InverseDeltaScalar = 1.f / (maxScalar - minScalar);
      else
        InverseDeltaScalar = minScalar;
    }
    typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1, _2, _3, _4);

    template <typename ScalarPortalType>
    VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Float32, 3>& rayDir,
                              const vtkm::Float32& minDistance, vtkm::Vec<vtkm::Float32, 4>& color,
                              ScalarPortalType& scalars) const
    {
      color[0] = 0.f;
      color[1] = 0.f;
      color[2] = 0.f;
      color[3] = 0.f;
      if (minDistance == -1.f)
        return; //TODO: Compact? or just image subset...
      //get the initial sample position;
      vtkm::Vec<vtkm::Float32, 3> sampleLocation;
      sampleLocation = CameraPosition + (0.0001f + minDistance) * rayDir;
      /*
              7----------6
             /|         /|
            4----------5 |
            | |        | |
            | 3--------|-2    z y
            |/         |/     |/
            0----------1      |__ x
      */
      vtkm::Vec<vtkm::Float32, 3> bottomLeft(0, 0, 0);
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

      vtkm::Vec<vtkm::Id, 3> cell(0, 0, 0);
      vtkm::Vec<vtkm::Float32, 3> invSpacing;

      while (Locator.IsInside(sampleLocation))
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

        vtkm::Vec<vtkm::Float32, 4> sampleColor = ColorMap.Get(colorIndex);

        //composite
        sampleColor[3] *= (1.f - color[3]);
        color[0] = color[0] + sampleColor[0] * sampleColor[3];
        color[1] = color[1] + sampleColor[1] * sampleColor[3];
        color[2] = color[2] + sampleColor[2] * sampleColor[3];
        color[3] = sampleColor[3] + color[3];
        //advance
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
    }
  }; //Sampler

  template <typename LocatorType>
  class SamplerCellAssoc : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<Device>::PortalConst ColorArrayPortal;
    vtkm::Vec<vtkm::Float32, 3> CameraPosition;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    vtkm::Float32 MinScalar;
    vtkm::Float32 SampleDistance;
    vtkm::Float32 InverseDeltaScalar;
    LocatorType Locator;

  public:
    VTKM_CONT
    SamplerCellAssoc(vtkm::Vec<vtkm::Float32, 3> cameraPosition, const ColorArrayHandle& colorMap,
                     const vtkm::Float32& minScalar, const vtkm::Float32& maxScalar,
                     const vtkm::Float32& sampleDistance, const LocatorType& locator)
      : CameraPosition(cameraPosition)
      , ColorMap(colorMap.PrepareForInput(Device()))
      , MinScalar(minScalar)
      , SampleDistance(sampleDistance)
      , Locator(locator)
    {
      ColorMapSize = colorMap.GetNumberOfValues() - 1;
      if ((maxScalar - minScalar) != 0.f)
        InverseDeltaScalar = 1.f / (maxScalar - minScalar);
      else
        InverseDeltaScalar = minScalar;
    }
    typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1, _2, _3, _4);

    template <typename ScalarPortalType>
    VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Float32, 3>& rayDir,
                              const vtkm::Float32& minDistance, vtkm::Vec<vtkm::Float32, 4>& color,
                              const ScalarPortalType& scalars) const
    {
      color[0] = 0.f;
      color[1] = 0.f;
      color[2] = 0.f;
      color[3] = 0.f;
      if (minDistance == -1.f)
        return; //TODO: Compact? or just image subset...
      //get the initial sample position;
      vtkm::Vec<vtkm::Float32, 3> sampleLocation;
      sampleLocation = CameraPosition + (0.0001f + minDistance) * rayDir;

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
      vtkm::Vec<vtkm::Float32, 4> sampleColor(0, 0, 0, 0);
      vtkm::Vec<vtkm::Float32, 3> bottomLeft(0, 0, 0);
      vtkm::Vec<vtkm::Float32, 3> invSpacing;
      vtkm::Vec<vtkm::Id, 3> cell(0, 0, 0);
      while (Locator.IsInside(sampleLocation))
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
    vtkm::Vec<vtkm::Float32, 3> CameraPosition;

  public:
    VTKM_CONT
    CalcRayStart(vtkm::Vec<vtkm::Float32, 3> cameraPosition, const vtkm::Bounds boundingBox)
      : CameraPosition(cameraPosition)
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

    typedef void ControlSignature(FieldIn<>, FieldOut<>, FieldOut<>);
    typedef void ExecutionSignature(_1, _2, _3);
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Float32, 3>& rayDir, vtkm::Float32& minDistance,
                    vtkm::Float32& maxDistance) const
    {
      vtkm::Float32 dirx = rayDir[0];
      vtkm::Float32 diry = rayDir[1];
      vtkm::Float32 dirz = rayDir[2];

      vtkm::Float32 invDirx = rcp_safe(dirx);
      vtkm::Float32 invDiry = rcp_safe(diry);
      vtkm::Float32 invDirz = rcp_safe(dirz);

      vtkm::Float32 odirx = CameraPosition[0] * invDirx;
      vtkm::Float32 odiry = CameraPosition[1] * invDiry;
      vtkm::Float32 odirz = CameraPosition[2] * invDirz;

      vtkm::Float32 xmin = Xmin * invDirx - odirx;
      vtkm::Float32 ymin = Ymin * invDiry - odiry;
      vtkm::Float32 zmin = Zmin * invDirz - odirz;
      vtkm::Float32 xmax = Xmax * invDirx - odirx;
      vtkm::Float32 ymax = Ymax * invDiry - odiry;
      vtkm::Float32 zmax = Zmax * invDirz - odirz;

      minDistance = vtkm::Max(
        vtkm::Max(vtkm::Max(vtkm::Min(ymin, ymax), vtkm::Min(xmin, xmax)), vtkm::Min(zmin, zmax)),
        0.f);
      maxDistance =
        vtkm::Min(vtkm::Min(vtkm::Max(ymin, ymax), vtkm::Max(xmin, xmax)), vtkm::Max(zmin, zmax));
      if (maxDistance < minDistance)
      {
        minDistance = -1.f; //flag for miss
      }
    }
  }; //class CalcRayStart

  class CompositeBackground : public vtkm::worklet::WorkletMapField
  {
    vtkm::Vec<vtkm::Float32, 4> BackgroundColor;

  public:
    VTKM_CONT
    CompositeBackground(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor)
      : BackgroundColor(backgroundColor)
    {
    }

    typedef void ControlSignature(FieldInOut<>);
    typedef void ExecutionSignature(_1);
    VTKM_EXEC
    void operator()(vtkm::Vec<vtkm::Float32, 4>& color) const
    {

      if (color[3] >= 1.f)
        return;
      vtkm::Float32 alpha = BackgroundColor[3] * (1.f - color[3]);
      color[0] = color[0] + BackgroundColor[0] * alpha;
      color[1] = color[1] + BackgroundColor[1] * alpha;
      color[2] = color[2] + BackgroundColor[2] * alpha;
      color[3] = alpha + color[3];
    }
  }; //class CompositeBackground

  VTKM_CONT
  VolumeRendererStructured()
  {
    IsSceneDirty = false;
    IsUniformDataSet = true;
    SampleDistance = -1.f;
    DoCompositeBackground = false;
  }

  VTKM_CONT
  Camera<Device>& GetCamera() { return camera; }

  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap)
  {
    ColorMap = colorMap;
  }

  VTKM_CONT
  void Init()
  {
    camera.CreateRays(Rays, DataBounds);
    IsSceneDirty = true;
  }

  VTKM_CONT
  void SetData(const vtkm::cont::CoordinateSystem& coords, const vtkm::cont::Field& scalarField,
               const vtkm::Bounds& coordsBounds, const vtkm::cont::CellSetStructured<3>& cellset,
               const vtkm::Range& scalarRange)
  {
    if (coords.GetData().IsSameType(CartesianArrayHandle()))
      IsUniformDataSet = false;
    IsSceneDirty = true;
    Coordinates = coords.GetData();
    ScalarField = &scalarField;
    DataBounds = coordsBounds;
    Cellset = cellset;
    ScalarRange = scalarRange;
  }

  VTKM_CONT
  void Render(CanvasRayTracer* canvas)
  {
    if (IsSceneDirty)
    {
      Init();
    }
    if (SampleDistance <= 0.f)
    {
      vtkm::Vec<vtkm::Float32, 3> extent;
      extent[0] = static_cast<vtkm::Float32>(this->DataBounds.X.Length());
      extent[1] = static_cast<vtkm::Float32>(this->DataBounds.Y.Length());
      extent[2] = static_cast<vtkm::Float32>(this->DataBounds.Z.Length());
      const vtkm::Float32 defaultNumberOfSamples = 200.f;
      SampleDistance = vtkm::Magnitude(extent) / defaultNumberOfSamples;
    }

    vtkm::worklet::DispatcherMapField<CalcRayStart, Device>(
      CalcRayStart(camera.GetPosition(), this->DataBounds))
      .Invoke(Rays.Dir, Rays.MinDistance, Rays.MaxDistance);
    bool isSupportedField = (ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS ||
                             ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET);
    if (!isSupportedField)
      throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
    bool isAssocPoints = ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS;

    if (IsUniformDataSet)
    {

      vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
      vertices = Coordinates.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      UniformLocator<Device> locator(vertices, Cellset);

      if (isAssocPoints)
      {
        vtkm::worklet::DispatcherMapField<Sampler<UniformLocator<Device>>, Device>(
          Sampler<UniformLocator<Device>>(camera.GetPosition(), ColorMap,
                                          vtkm::Float32(ScalarRange.Min),
                                          vtkm::Float32(ScalarRange.Max), SampleDistance, locator))
          .Invoke(Rays.Dir, Rays.MinDistance, camera.FrameBuffer, *ScalarField);
      }
      else
      {
        vtkm::worklet::DispatcherMapField<SamplerCellAssoc<UniformLocator<Device>>>(
          SamplerCellAssoc<UniformLocator<Device>>(
            camera.GetPosition(), ColorMap, vtkm::Float32(ScalarRange.Min),
            vtkm::Float32(ScalarRange.Max), SampleDistance, locator))
          .Invoke(Rays.Dir, Rays.MinDistance, camera.FrameBuffer, *ScalarField);
      }
    }
    else
    {
      CartesianArrayHandle vertices;
      vertices = Coordinates.Cast<CartesianArrayHandle>();
      RectilinearLocator<Device> locator(vertices, Cellset);
      if (isAssocPoints)
      {
        vtkm::worklet::DispatcherMapField<Sampler<RectilinearLocator<Device>>, Device>(
          Sampler<RectilinearLocator<Device>>(
            camera.GetPosition(), ColorMap, vtkm::Float32(ScalarRange.Min),
            vtkm::Float32(ScalarRange.Max), SampleDistance, locator))
          .Invoke(Rays.Dir, Rays.MinDistance, camera.FrameBuffer, *ScalarField);
      }
      else
      {
        vtkm::worklet::DispatcherMapField<SamplerCellAssoc<RectilinearLocator<Device>>, Device>(
          SamplerCellAssoc<RectilinearLocator<Device>>(
            camera.GetPosition(), ColorMap, vtkm::Float32(ScalarRange.Min),
            vtkm::Float32(ScalarRange.Max), SampleDistance, locator))
          .Invoke(Rays.Dir, Rays.MinDistance, camera.FrameBuffer, *ScalarField);
      }
    }

    if (DoCompositeBackground)
    {
      vtkm::worklet::DispatcherMapField<CompositeBackground>(CompositeBackground(BackgroundColor))
        .Invoke(camera.FrameBuffer);
    }
    camera.WriteToSurface(canvas, Rays.MinDistance);
  } //Render

  VTKM_CONT
  void SetSampleDistance(const vtkm::Float32& distance)
  {
    if (distance <= 0.f)
      throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
    SampleDistance = distance;
  }

  VTKM_CONT
  void SetBackgroundColor(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor)
  {
    BackgroundColor = backgroundColor;
  }

  void SetCompositeBackground(bool compositeBackground)
  {
    DoCompositeBackground = compositeBackground;
  }

protected:
  bool IsSceneDirty;
  bool IsUniformDataSet;
  bool DoCompositeBackground;
  VolumeRay<Device> Rays;
  Camera<Device> camera;
  vtkm::cont::DynamicArrayHandleCoordinateSystem Coordinates;
  vtkm::cont::CellSetStructured<3> Cellset;
  const vtkm::cont::Field* ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorMap;
  vtkm::Float32 SampleDistance;
  vtkm::Vec<vtkm::Float32, 4> BackgroundColor;
  vtkm::Range ScalarRange;
  vtkm::Bounds DataBounds;
};
}
}
} //namespace vtkm::rendering::raytracing
#endif
