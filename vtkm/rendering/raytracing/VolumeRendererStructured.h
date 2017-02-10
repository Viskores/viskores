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

#include <math.h>
#include <iostream>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm {
namespace rendering{
namespace raytracing{

template< typename DeviceAdapter>
class VolumeRendererStructured
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
  typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle,DefaultHandle,DefaultHandle> CartesianArrayHandle;

  class Sampler : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;
    //vtkm::Float32 BoundingBox[6];
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Vec<vtkm::Float32,3> Origin;
    vtkm::Vec<vtkm::Float32,3> InvSpacing;
    vtkm::Id3 PointDimensions;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    UniformConstPortal Coordinates;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,3> Conn;
    vtkm::Float32 MinScalar;
    vtkm::Float32 SampleDistance;
    vtkm::Float32 InverseDeltaScalar;
  public:
    VTKM_CONT
    Sampler(vtkm::Vec<vtkm::Float32,3> cameraPosition,
            const ColorArrayHandle &colorMap,
            const UniformArrayHandle &coordinates,
            vtkm::cont::CellSetStructured<3> &cellset,
            const vtkm::Float32 &minScalar,
            const vtkm::Float32 &maxScalar,
            const vtkm::Float32 &sampleDistance)
      : CameraPosition(cameraPosition),
        ColorMap( colorMap.PrepareForInput( DeviceAdapter() )),
        Coordinates(coordinates.PrepareForInput( DeviceAdapter() )),
        Conn( cellset.PrepareForInput( DeviceAdapter(),
                                       vtkm::TopologyElementTagPoint(),
                                       vtkm::TopologyElementTagCell() )),
        MinScalar(minScalar),
        SampleDistance(sampleDistance)
    {
      ColorMapSize = colorMap.GetNumberOfValues() - 1;

      Origin = Coordinates.GetOrigin();
      PointDimensions = Conn.GetPointDimensions();
      vtkm::Vec<vtkm::Float32,3> spacing = Coordinates.GetSpacing();
      InvSpacing[0] = 1.f / spacing[0];
      InvSpacing[1] = 1.f / spacing[1];
      InvSpacing[2] = 1.f / spacing[2];
      if((maxScalar - minScalar) != 0.f) InverseDeltaScalar = 1.f / (maxScalar - minScalar);
      else InverseDeltaScalar = minScalar;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    _5);
    VTKM_EXEC
    void
    LocateCell(const vtkm::Vec<vtkm::Float32,3> &point,
               vtkm::Vec<vtkm::Id,8> &cellIndices) const
    {
      vtkm::Vec<vtkm::Float32,3> temp = point;
      //make sure that if we border the upper edge, we sample the correct cell
      if(temp[0] == vtkm::Float32(PointDimensions[0] - 1)) temp[0] = vtkm::Float32(PointDimensions[0] - 2);
      if(temp[1] == vtkm::Float32(PointDimensions[1] - 1)) temp[0] = vtkm::Float32(PointDimensions[1] - 2);
      if(temp[2] == vtkm::Float32(PointDimensions[2] - 1)) temp[0] = vtkm::Float32(PointDimensions[2] - 2);
      temp = temp - Origin;
      temp = temp * InvSpacing;
      vtkm::Vec<vtkm::Id,3> cell = temp;
      //TODO: Just do this manually, this just does un-needed calcs
      //cellId = Conn.LogicalToFlatCellIndex(cell);
      //cellIndices = Conn.GetIndices(cellId);
      cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
      cellIndices[1] = cellIndices[0] + 1;
      cellIndices[2] = cellIndices[1] + PointDimensions[0];
      cellIndices[3] = cellIndices[2] - 1;
      cellIndices[4] = cellIndices[0] + PointDimensions[0]*PointDimensions[1];
      cellIndices[5] = cellIndices[4] + 1;
      cellIndices[6] = cellIndices[5] + PointDimensions[0];
      cellIndices[7] = cellIndices[6] - 1;
    }

    template<typename ScalarPortalType>
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Float32,3> &rayDir,
                    const vtkm::Float32 &minDistance,
                    const vtkm::Float32 &maxDistance,
                    vtkm::Vec<vtkm::Float32,4> &color,
                    ScalarPortalType &scalars) const
    {
      color[0] = 0.f;
      color[1] = 0.f;
      color[2] = 0.f;
      color[3] = 0.f;
      if(minDistance == -1.f) return; //TODO: Compact? or just image subset...
      //get the initial sample position;
      vtkm::Float32 currentDistance = minDistance + SampleDistance; //Move the ray forward some epsilon
      vtkm::Float32 lastSample = maxDistance - SampleDistance;
      vtkm::Vec<vtkm::Float32,3> sampleLocation = CameraPosition + currentDistance * rayDir;
      /*
              7----------6
             /|         /|
            4----------5 |
            | |        | |
            | 3--------|-2    z y
            |/         |/     |/
            0----------1      |__ x
      */
      vtkm::Vec<vtkm::Float32,3> bottomLeft(0,0,0);
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

      while(currentDistance < lastSample)
      {

        if(sampleLocation[0] < Origin[0] || sampleLocation[0] >= vtkm::Float32(PointDimensions[0] - 1)) return;
        if(sampleLocation[1] < Origin[1] || sampleLocation[1] >= vtkm::Float32(PointDimensions[1] - 1)) return;
        if(sampleLocation[2] < Origin[2] || sampleLocation[2] >= vtkm::Float32(PointDimensions[2] - 1)) return;
        if( tx > 1.f || tx < 0.f) newCell = true;
        if( ty > 1.f || ty < 0.f) newCell = true;
        if( tz > 1.f || tz < 0.f) newCell = true;

        if(newCell)
        {
          vtkm::Vec<vtkm::Id,8> cellIndices;
          LocateCell(sampleLocation, cellIndices);

          bottomLeft = Coordinates.Get(cellIndices[0]);

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

          tx = (sampleLocation[0] - bottomLeft[0]) * InvSpacing[0];
          ty = (sampleLocation[1] - bottomLeft[1]) * InvSpacing[1];
          tz = (sampleLocation[2] - bottomLeft[2]) * InvSpacing[2];

          newCell = false;
        }

        vtkm::Float32 lerped76 = scalar7 + tx * scalar6minus7;
        vtkm::Float32 lerped45 = scalar4 + tx * scalar5minus4;
        vtkm::Float32 lerpedTop = lerped45 + ty * (lerped76 - lerped45);

        vtkm::Float32 lerped01 = scalar0 + tx * scalar1minus0;
        vtkm::Float32 lerped32 = scalar3 + tx * scalar2minus3;
        vtkm::Float32 lerpedBottom = lerped01 + ty * (lerped32 - lerped01);

        vtkm::Float32 finalScalar = lerpedBottom + tz *(lerpedTop - lerpedBottom);
        //normalize scalar
        finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

        vtkm::Id colorIndex = static_cast<vtkm::Id>(
              finalScalar * static_cast<vtkm::Float32>(ColorMapSize));
        //colorIndex = vtkm::Min(ColorMapSize, vtkm::Max(0,colorIndex));
        vtkm::Vec<vtkm::Float32,4> sampleColor = ColorMap.Get(colorIndex);
        //sampleColor[3] = .05f;

        //composite
        sampleColor[3] *= (1.f - color[3]);
        color[0] = color[0] + sampleColor[0] * sampleColor[3];
        color[1] = color[1] + sampleColor[1] * sampleColor[3];
        color[2] = color[2] + sampleColor[2] * sampleColor[3];
        color[3] = sampleColor[3] + color[3];
        //advance
        currentDistance += SampleDistance;
        sampleLocation = sampleLocation + SampleDistance * rayDir;

        //this is linear could just do an addition
        tx = (sampleLocation[0] - bottomLeft[0]) * InvSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * InvSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * InvSpacing[2];

        // tx += deltaTx;
        // ty += deltaTy;
        // tz += deltaTz;
        if(color[3] >= 1.f) break;
      }
      //color[0] = vtkm::Min(color[0],1.f);
      //color[1] = vtkm::Min(color[1],1.f);
      //color[2] = vtkm::Min(color[2],1.f);
    }
  }; //Sampler

  class SamplerCellAssoc : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;
    //vtkm::Float32 BoundingBox[6];
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Vec<vtkm::Float32,3> Origin;
    vtkm::Vec<vtkm::Float32,3> InvSpacing;
    vtkm::Id3 CellDimensions;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    UniformConstPortal Coordinates;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,3> Conn;
    vtkm::Float32 MinScalar;
    vtkm::Float32 SampleDistance;
    vtkm::Float32 InverseDeltaScalar;
  public:
    VTKM_CONT
    SamplerCellAssoc(vtkm::Vec<vtkm::Float32,3> cameraPosition,
                     const ColorArrayHandle &colorMap,
                     const UniformArrayHandle &coordinates,
                     vtkm::cont::CellSetStructured<3> &cellset,
                     const vtkm::Float32 &minScalar,
                     const vtkm::Float32 &maxScalar,
                     const vtkm::Float32 &sampleDistance)
      : CameraPosition(cameraPosition),
        ColorMap( colorMap.PrepareForInput( DeviceAdapter() )),
        Coordinates(coordinates.PrepareForInput( DeviceAdapter() )),
        Conn( cellset.PrepareForInput( DeviceAdapter(),
                                       vtkm::TopologyElementTagPoint(),
                                       vtkm::TopologyElementTagCell() )),
        MinScalar(minScalar),
        SampleDistance(sampleDistance)
    {
      ColorMapSize = colorMap.GetNumberOfValues() - 1;

      Origin = Coordinates.GetOrigin();
      CellDimensions = Conn.GetPointDimensions();
      CellDimensions[0] -= 1;
      CellDimensions[1] -= 1;
      CellDimensions[2] -= 1;
      vtkm::Vec<vtkm::Float32,3> spacing = Coordinates.GetSpacing();
      InvSpacing[0] = 1.f / spacing[0];
      InvSpacing[1] = 1.f / spacing[1];
      InvSpacing[2] = 1.f / spacing[2];
      if((maxScalar - minScalar) != 0.f) InverseDeltaScalar = 1.f / (maxScalar - minScalar);
      else InverseDeltaScalar = minScalar;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    _5);
    VTKM_EXEC
    void
    LocateCellId(const vtkm::Vec<vtkm::Float32,3> &point,
                 vtkm::Id &cellId) const
    {
      vtkm::Vec<vtkm::Float32,3> temp = point;
      temp = temp - Origin;
      temp = temp * InvSpacing;
      if(temp[0] == vtkm::Float32(CellDimensions[0])) temp[0] = vtkm::Float32(CellDimensions[0] - 1);
      if(temp[1] == vtkm::Float32(CellDimensions[1])) temp[0] = vtkm::Float32(CellDimensions[1] - 1);
      if(temp[2] == vtkm::Float32(CellDimensions[2])) temp[0] = vtkm::Float32(CellDimensions[2] - 1);
      vtkm::Vec<vtkm::Id,3> cell = temp;
      cellId = (cell[2] * CellDimensions[1] + cell[1]) * CellDimensions[0] + cell[0];
    }

    template<typename ScalarPortalType>
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Float32,3> &rayDir,
                    const vtkm::Float32 &minDistance,
                    const vtkm::Float32 &maxDistance,
                    vtkm::Vec<vtkm::Float32,4> &color,
                    const ScalarPortalType &scalars) const
    {
      color[0] = 0.f;
      color[1] = 0.f;
      color[2] = 0.f;
      color[3] = 0.f;
      if(minDistance == -1.f) return; //TODO: Compact? or just image subset...
      //get the initial sample position;
      vtkm::Float32 currentDistance = minDistance + 0.001f; //Move the ray forward some epsilon
      vtkm::Float32 lastSample = maxDistance - 0.001f;
      vtkm::Vec<vtkm::Float32,3> sampleLocation = CameraPosition + currentDistance * rayDir;

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
      //check to see if we left the cell
//      vtkm::Float32 deltaTx = SampleDistance * rayDir[0] * InvSpacing[0];
//      vtkm::Float32 deltaTy = SampleDistance * rayDir[1] * InvSpacing[1];
//      vtkm::Float32 deltaTz = SampleDistance * rayDir[2] * InvSpacing[2];
      vtkm::Float32 tx = 2.f;
      vtkm::Float32 ty = 2.f;
      vtkm::Float32 tz = 2.f;
      vtkm::Float32 scalar0 = 0.f;
      vtkm::Vec<vtkm::Float32,4> sampleColor(0,0,0,0);
      vtkm::Vec<vtkm::Float32,3> bottomLeft(0,0,0);
      while(currentDistance < lastSample)
      {
        if( tx > 1.f ) newCell = true;
        if( ty > 1.f ) newCell = true;
        if( tz > 1.f ) newCell = true;

        if(newCell)
        {
          vtkm::Id cellId;

          LocateCellId(sampleLocation, cellId);
          scalar0 = vtkm::Float32(scalars.Get(cellId));
          vtkm::Float32 normalScalar = (scalar0 - MinScalar) * InverseDeltaScalar;
          sampleColor = ColorMap.Get(static_cast<vtkm::Id>(normalScalar *
                                         static_cast<vtkm::Float32>(ColorMapSize)));
          bottomLeft = Coordinates.Get(cellId);
          tx = (sampleLocation[0] - bottomLeft[0]) * InvSpacing[0];
          ty = (sampleLocation[1] - bottomLeft[1]) * InvSpacing[1];
          tz = (sampleLocation[2] - bottomLeft[2]) * InvSpacing[2];
          newCell = false;
        }

        // just repeatably composite
        vtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
        color[0] = color[0] + sampleColor[0] * alpha;
        color[1] = color[1] + sampleColor[1] * alpha;
        color[2] = color[2] + sampleColor[2] * alpha;
        color[3] = alpha + color[3];
        //advance
        currentDistance += SampleDistance;

        if(color[3] >= 1.f) break;
        tx = (sampleLocation[0] - bottomLeft[0]) * InvSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * InvSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * InvSpacing[2];

      }

    }
  }; //SamplerCell
class SamplerCellAssocRect : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename DefaultHandle::ExecutionTypes<DeviceAdapter>::PortalConst DefaultConstHandle;
    typedef typename CartesianArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst CartesianConstPortal;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;
    //vtkm::Float32 BoundingBox[6];
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Id3 PointDimensions;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    CartesianConstPortal Coordinates;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,3> Conn;
    vtkm::Float32 MinScalar;
    vtkm::Float32 SampleDistance;
    vtkm::Float32 InverseDeltaScalar;
    DefaultConstHandle CoordPortals[3];

  public:
    VTKM_CONT
    SamplerCellAssocRect(vtkm::Vec<vtkm::Float32,3> cameraPosition,
                         const ColorArrayHandle &colorMap,
                         const CartesianArrayHandle &coordinates,
                         vtkm::cont::CellSetStructured<3> &cellset,
                         const vtkm::Float32 &minScalar,
                         const vtkm::Float32 &maxScalar,
                         const vtkm::Float32 &sampleDistance)
      : CameraPosition(cameraPosition),
        ColorMap( colorMap.PrepareForInput( DeviceAdapter() )),
        Coordinates(coordinates.PrepareForInput( DeviceAdapter() )),
        Conn( cellset.PrepareForInput( DeviceAdapter(),
                                       vtkm::TopologyElementTagPoint(),
                                       vtkm::TopologyElementTagCell() )),
        MinScalar(minScalar),
        SampleDistance(sampleDistance)
    {
      CoordPortals[0] = Coordinates.GetFirstPortal();
      CoordPortals[1] = Coordinates.GetSecondPortal();
      CoordPortals[2] = Coordinates.GetThirdPortal();
      ColorMapSize = colorMap.GetNumberOfValues() - 1;
      PointDimensions = Conn.GetPointDimensions();

      if((maxScalar - minScalar) != 0.f) InverseDeltaScalar = 1.f / (maxScalar - minScalar);
      else InverseDeltaScalar = minScalar;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    _5);
    // Locate assumes that the point is within the data set which
    // should be true when the min and max distance are passed in
    // This is a linear search from the previous cell loc
   VTKM_EXEC
    void
    LocateCell(vtkm::Vec<vtkm::Id,3> &cell,
               const vtkm::Vec<vtkm::Float32,3> &point,
               const vtkm::Vec<vtkm::Float32,3> &rayDir,
               vtkm::Float32 *invSpacing) const
      {
        for(vtkm::Int32 dim = 0; dim < 3; ++dim)
        {
          if(rayDir[dim] == 0.f) continue;
          vtkm::FloatDefault searchDir = (rayDir[dim] > 0.f) ? vtkm::FloatDefault(1.0) : vtkm::FloatDefault(-1.0);
          bool notFound = true;
          while(notFound)
          {
            vtkm::Id nextPoint = cell[dim] + static_cast<vtkm::Id>(searchDir);
            bool validPoint = true;
            if(nextPoint < 0 || nextPoint > PointDimensions[dim]) validPoint = false;
            if( validPoint && (CoordPortals[dim].Get( nextPoint ) * searchDir < point[dim] * searchDir))
            {
              cell[dim] += vtkm::Id(searchDir);
            }
            else notFound = false;
          }
          invSpacing[dim] = 1.f / static_cast<vtkm::Float32>(CoordPortals[dim].Get(cell[dim]+1) - CoordPortals[dim].Get(cell[dim]));
      }
    }

    template<typename ScalarPortalType>
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Float32,3> &rayDir,
                    const vtkm::Float32 &minDistance,
                    const vtkm::Float32 &maxDistance,
                    vtkm::Vec<vtkm::Float32,4> &color,
                    ScalarPortalType &scalars) const
    {
      color[0] = 0.f;
      color[1] = 0.f;
      color[2] = 0.f;
      color[3] = 0.f;
      vtkm::Vec<Id,3> currentCell;
      if(minDistance == -1.f) return; //TODO: Compact? or just image subset...
        //TODO: Just let it search for now. There are better ways of doing this
      currentCell[0] = (rayDir[0] < 0) ? PointDimensions[0] - 2 : 0;
      currentCell[1] = (rayDir[1] < 0) ? PointDimensions[1] - 2 : 0;
      currentCell[2] = (rayDir[2] < 0) ? PointDimensions[2] - 2 : 0;
      //get the initial sample position;
      vtkm::Float32 currentDistance = minDistance + SampleDistance; //Move the ray forward some epsilon
      vtkm::Float32 lastSample = maxDistance - SampleDistance;
      vtkm::Vec<vtkm::Float32,3> sampleLocation = CameraPosition + currentDistance * rayDir;
      vtkm::Float32 invSpacing[3];
      vtkm::Vec<vtkm::Id,8> cellIndices;
      /*LocateCell(currentCell,
                 sampleLocation,
                 rayDir,
                 invSpacing);
      GetCellIndices(currentCell, cellIndices);
      */
      /*
              7----------6
             /|         /|
            4----------5 |
            | |        | |
            | 3--------|-2    z y
            |/         |/     |/
            0----------1      |__ x
      */
      vtkm::Vec<vtkm::Float32,3> bottomLeft(0,0,0);
      bool newCell = true;
      //check to see if we left the cell
      vtkm::Float32 tx = 0.f;
      vtkm::Float32 ty = 0.f;
      vtkm::Float32 tz = 0.f;
      vtkm::Float32 scalar0 = 0.f;

      while(currentDistance < lastSample)
      {

        if( tx > 1.f || tx < 0.f) newCell = true;
        if( ty > 1.f || ty < 0.f) newCell = true;
        if( tz > 1.f || tz < 0.f) newCell = true;

        if(newCell)
        {


          LocateCell(currentCell,
                     sampleLocation,
                     rayDir,
                     invSpacing);

          vtkm::Id cellIdx = (currentCell[2] * (PointDimensions[1]-1) + currentCell[1]) * (PointDimensions[0]-1) + currentCell[0];
          bottomLeft = Coordinates.Get(cellIdx);
          scalar0 = vtkm::Float32(scalars.Get(cellIdx));

          tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
          ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
          tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

          newCell = false;
        }


        //normalize scalar
        scalar0 = (scalar0 - MinScalar) * InverseDeltaScalar;

        vtkm::Id colorIndex;
        colorIndex = static_cast<vtkm::Id>(scalar0 *
                                           static_cast<vtkm::Float32>(ColorMapSize));
        colorIndex = vtkm::Min(ColorMapSize, vtkm::Max(vtkm::Id(0),colorIndex));
        vtkm::Vec<vtkm::Float32,4> sampleColor = ColorMap.Get(colorIndex);
        //sampleColor[3] = .05f;

        //composite
        sampleColor[3] *= (1.f - color[3]);
        color[0] = color[0] + sampleColor[0] * sampleColor[3];
        color[1] = color[1] + sampleColor[1] * sampleColor[3];
        color[2] = color[2] + sampleColor[2] * sampleColor[3];
        color[3] = sampleColor[3] + color[3];
        //advance
        currentDistance += SampleDistance;
        sampleLocation = sampleLocation + SampleDistance * rayDir;

        //this is linear could just do an addition
        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

        // tx += deltaTx;
        // ty += deltaTy;
        // tz += deltaTz;
        if(color[3] >= 1.f) break;
      }
    }
  }; //SamplerCellRect

  class SamplerRect : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename DefaultHandle::ExecutionTypes<DeviceAdapter>::PortalConst DefaultConstHandle;
    typedef typename CartesianArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst CartesianConstPortal;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Id3 PointDimensions;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    CartesianConstPortal Coordinates;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,3> Conn;
    vtkm::Float32 MinScalar;
    vtkm::Float32 SampleDistance;
    vtkm::Float32 InverseDeltaScalar;
    DefaultConstHandle CoordPortals[3];

  public:
    VTKM_CONT
    SamplerRect(vtkm::Vec<vtkm::Float32,3> cameraPosition,
                const ColorArrayHandle &colorMap,
                const CartesianArrayHandle &coordinates,
                vtkm::cont::CellSetStructured<3> &cellset,
                const vtkm::Float32 &minScalar,
                const vtkm::Float32 &maxScalar,
                const vtkm::Float32 &sampleDistance)
      : CameraPosition(cameraPosition),
        ColorMap( colorMap.PrepareForInput( DeviceAdapter() )),
        Coordinates(coordinates.PrepareForInput( DeviceAdapter() )),
        Conn( cellset.PrepareForInput( DeviceAdapter(),
                                       vtkm::TopologyElementTagPoint(),
                                       vtkm::TopologyElementTagCell() )),
        MinScalar(minScalar),
        SampleDistance(sampleDistance)
    {
      CoordPortals[0] = Coordinates.GetFirstPortal();
      CoordPortals[1] = Coordinates.GetSecondPortal();
      CoordPortals[2] = Coordinates.GetThirdPortal();
      ColorMapSize = colorMap.GetNumberOfValues() - 1;
      ColorMapSize = colorMap.GetNumberOfValues() - 1;
      ColorMapSize = colorMap.GetNumberOfValues() - 1;
      PointDimensions = Conn.GetPointDimensions();
      if((maxScalar - minScalar) != 0.f) InverseDeltaScalar = 1.f / (maxScalar - minScalar);
      else InverseDeltaScalar = minScalar;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    _5);
    VTKM_EXEC
    void
    GetCellIndices(const vtkm::Vec<vtkm::Id,3> cell,
               vtkm::Vec<vtkm::Id,8> &cellIndices) const
    {
      cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
      cellIndices[1] = cellIndices[0] + 1;
      cellIndices[2] = cellIndices[1] + PointDimensions[0];
      cellIndices[3] = cellIndices[2] - 1;
      cellIndices[4] = cellIndices[0] + PointDimensions[0]*PointDimensions[1];
      cellIndices[5] = cellIndices[4] + 1;
      cellIndices[6] = cellIndices[5] + PointDimensions[0];
      cellIndices[7] = cellIndices[6] - 1;
    }

    //
    // Locate assumes that the point is within the data set which
    // should be true when the min and max distance are passed in
    //
    VTKM_EXEC
    void
    LocateCell(vtkm::Vec<vtkm::Id,3> &cell,
               const vtkm::Vec<vtkm::Float32,3> &point,
               const vtkm::Vec<vtkm::Float32,3> &rayDir,
               vtkm::Float32 *invSpacing) const
      {
        for(vtkm::Int32 dim = 0; dim < 3; ++dim)
        {
          bool notFound = true;
          if(rayDir[dim] == 0.f)
          {
            // If the ray direction is zero, then the ray does not move from
            // cell to cell, and is therefore already found.
            notFound = false;
          }
          vtkm::FloatDefault searchDir = (rayDir[dim] > 0.f) ? vtkm::FloatDefault(1.0) : vtkm::FloatDefault(-1.0);
          while(notFound)
          {
            vtkm::Id nextPoint = cell[dim] + static_cast<vtkm::Id>(searchDir);
            bool validPoint = true;
            if(nextPoint < 0 || nextPoint > PointDimensions[dim]) validPoint = false;
            if( validPoint && (CoordPortals[dim].Get( nextPoint ) * searchDir < point[dim] * searchDir))
            {
              cell[dim] += vtkm::Id(searchDir);
            }
            else notFound = false;
          }
          invSpacing[dim] = 1.f / static_cast<vtkm::Float32>(CoordPortals[dim].Get(cell[dim]+1) - CoordPortals[dim].Get(cell[dim]));
      }
    }

    template<typename ScalarPortalType>
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Float32,3> &rayDir,
                    const vtkm::Float32 &minDistance,
                    const vtkm::Float32 &maxDistance,
                    vtkm::Vec<vtkm::Float32,4> &color,
                    ScalarPortalType &scalars) const
    {
      color[0] = 0.f;
      color[1] = 0.f;
      color[2] = 0.f;
      color[3] = 0.f;
      vtkm::Vec<Id,3> currentCell;
      if(minDistance == -1.f) return; //TODO: Compact? or just image subset...
        //TODO: Just let it search for now. There are better ways of doing this
      //Also it will fail ray dir is 0
      currentCell[0] = (rayDir[0] < 0) ? PointDimensions[0] - 2 : 0;
      currentCell[1] = (rayDir[1] < 0) ? PointDimensions[1] - 2 : 0;
      currentCell[2] = (rayDir[2] < 0) ? PointDimensions[2] - 2 : 0;
      //get the initial sample position;
      vtkm::Float32 currentDistance = minDistance + SampleDistance; //Move the ray forward some epsilon
      vtkm::Float32 lastSample = maxDistance - SampleDistance;
      vtkm::Vec<vtkm::Float32,3> sampleLocation = CameraPosition + currentDistance * rayDir;
      vtkm::Float32 invSpacing[3];
      vtkm::Vec<vtkm::Id,8> cellIndices;

      /*
              7----------6
             /|         /|
            4----------5 |
            | |        | |
            | 3--------|-2    z y
            |/         |/     |/
            0----------1      |__ x
      */
      vtkm::Vec<vtkm::Float32,3> bottomLeft(0,0,0);
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

      while(currentDistance < lastSample)
      {
        if( tx > 1.f || tx < 0.f) newCell = true;
        if( ty > 1.f || ty < 0.f) newCell = true;
        if( tz > 1.f || tz < 0.f) newCell = true;

        if(newCell)
        {


          LocateCell(currentCell,
                     sampleLocation,
                     rayDir,
                     invSpacing);

          GetCellIndices(currentCell, cellIndices);
          bottomLeft = Coordinates.Get(cellIndices[0]);

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

        vtkm::Float32 finalScalar = lerpedBottom + tz *(lerpedTop - lerpedBottom);
        //normalize scalar
        finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

        vtkm::Id colorIndex;
        colorIndex = static_cast<vtkm::Id>(finalScalar *
                                           static_cast<vtkm::Float32>(ColorMapSize));
        //colorIndex = vtkm::Min(ColorMapSize, vtkm::Max(0,colorIndex));
        vtkm::Vec<vtkm::Float32,4> sampleColor = ColorMap.Get(colorIndex);

        //composite
        sampleColor[3] *= (1.f - color[3]);
        color[0] = color[0] + sampleColor[0] * sampleColor[3];
        color[1] = color[1] + sampleColor[1] * sampleColor[3];
        color[2] = color[2] + sampleColor[2] * sampleColor[3];
        color[3] = sampleColor[3] + color[3];
        //advance
        currentDistance += SampleDistance;
        sampleLocation = sampleLocation + SampleDistance * rayDir;

        //this is linear could just do an addition
        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

        // tx += deltaTx;
        // ty += deltaTy;
        // tz += deltaTz;
        if(color[3] >= 1.f) break;
      }
    }
  }; //SamplerRect

  class CalcRayStart : public vtkm::worklet::WorkletMapField
  {
   vtkm::Float32 Xmin;
   vtkm::Float32 Ymin;
   vtkm::Float32 Zmin;
   vtkm::Float32 Xmax;
   vtkm::Float32 Ymax;
   vtkm::Float32 Zmax;
   vtkm::Vec<vtkm::Float32,3> CameraPosition;
  public:
    VTKM_CONT
    CalcRayStart(vtkm::Vec<vtkm::Float32,3> cameraPosition,
                 const vtkm::Bounds boundingBox)
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
    vtkm::Float32 rcp(vtkm::Float32 f)  const { return 1.0f/f;}

    VTKM_EXEC
    vtkm::Float32 rcp_safe(vtkm::Float32 f) const { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }

    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3);
    VTKM_EXEC
    void operator()(const vtkm::Vec<vtkm::Float32,3> &rayDir,
                    vtkm::Float32 &minDistance,
                    vtkm::Float32 &maxDistance) const
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


      minDistance = vtkm::Max(vtkm::Max(vtkm::Max(vtkm::Min(ymin,ymax),vtkm::Min(xmin,xmax)),vtkm::Min(zmin,zmax)), 0.f);
      maxDistance = vtkm::Min(vtkm::Min(vtkm::Max(ymin,ymax),vtkm::Max(xmin,xmax)),vtkm::Max(zmin,zmax));
      if(maxDistance < minDistance)
      {
        minDistance = -1.f; //flag for miss
      }


    }
  }; //class CalcRayStart

  class CompositeBackground : public vtkm::worklet::WorkletMapField
  {
   vtkm::Vec<vtkm::Float32,4> BackgroundColor;
  public:
    VTKM_CONT
    CompositeBackground(const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
     : BackgroundColor(backgroundColor)
    {}

    typedef void ControlSignature(FieldInOut<>);
    typedef void ExecutionSignature(_1);
    VTKM_EXEC
    void operator()(vtkm::Vec<vtkm::Float32,4> &color) const
    {

        if(color[3] >= 1.f) return;
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
  Camera<DeviceAdapter>& GetCamera()
  {
    return camera;
  }

  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colorMap)
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
  void SetData(const vtkm::cont::CoordinateSystem &coords,
               const vtkm::cont::Field &scalarField,
               const vtkm::Bounds &coordsBounds,
               const vtkm::cont::CellSetStructured<3> &cellset,
               const vtkm::Range &scalarRange)
  {
    if(coords.GetData().IsSameType(CartesianArrayHandle())) IsUniformDataSet = false;
    IsSceneDirty = true;
    Coordinates = coords.GetData();
    ScalarField = &scalarField;
    DataBounds = coordsBounds;
    Cellset = cellset;
    ScalarRange = scalarRange;
  }


  VTKM_CONT
  void Render(CanvasRayTracer *canvas)
  {
    if(IsSceneDirty)
    {
      Init();
    }
    if(SampleDistance <= 0.f)
    {
      vtkm::Vec<vtkm::Float32,3> extent;
      extent[0] = static_cast<vtkm::Float32>(this->DataBounds.X.Length());
      extent[1] = static_cast<vtkm::Float32>(this->DataBounds.Y.Length());
      extent[2] = static_cast<vtkm::Float32>(this->DataBounds.Z.Length());
      SampleDistance = vtkm::Magnitude(extent) / 1000.f;
    }

    vtkm::worklet::DispatcherMapField< CalcRayStart >( CalcRayStart( camera.GetPosition(), this->DataBounds ))
      .Invoke( Rays.Dir,
               Rays.MinDistance,
               Rays.MaxDistance);
    bool isSupportedField = (ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS ||
                             ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET );
    if(!isSupportedField) throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
    bool isAssocPoints = ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS;
    if(IsUniformDataSet)
    {

      vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
      vertices = Coordinates.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      if(isAssocPoints)
      {
        vtkm::worklet::DispatcherMapField< Sampler >( Sampler( camera.GetPosition(),
                                                               ColorMap,
                                                               vertices,
                                                               Cellset,
                                                               vtkm::Float32(ScalarRange.Min),
                                                               vtkm::Float32(ScalarRange.Max),
                                                               SampleDistance ))
          .Invoke( Rays.Dir,
                   Rays.MinDistance,
                   Rays.MaxDistance,
                   camera.FrameBuffer,
                   *ScalarField);
      }
      else
      {
        vtkm::worklet::DispatcherMapField< SamplerCellAssoc >( SamplerCellAssoc( camera.GetPosition(),
                                                                                 ColorMap,
                                                                                 vertices,
                                                                                 Cellset,
                                                                                 vtkm::Float32(ScalarRange.Min),
                                                                                 vtkm::Float32(ScalarRange.Max),
                                                                                 SampleDistance ))
          .Invoke( Rays.Dir,
                   Rays.MinDistance,
                   Rays.MaxDistance,
                   camera.FrameBuffer,
                   *ScalarField);
      }
     }
     else
     {

        CartesianArrayHandle vertices;
        vertices = Coordinates.Cast<CartesianArrayHandle>();
        if(isAssocPoints)
        {
          vtkm::worklet::DispatcherMapField< SamplerRect >( SamplerRect( camera.GetPosition(),
                                                                         ColorMap,
                                                                         vertices,
                                                                         Cellset,
                                                                         vtkm::Float32(ScalarRange.Min),
                                                                         vtkm::Float32(ScalarRange.Max),
                                                                         SampleDistance ))
            .Invoke( Rays.Dir,
                     Rays.MinDistance,
                     Rays.MaxDistance,
                     camera.FrameBuffer,
                     *ScalarField);
        }
        else
        {
          vtkm::worklet::DispatcherMapField< SamplerCellAssocRect >( SamplerCellAssocRect( camera.GetPosition(),
                                                                                           ColorMap,
                                                                                           vertices,
                                                                                           Cellset,
                                                                                           vtkm::Float32(ScalarRange.Min),
                                                                                           vtkm::Float32(ScalarRange.Max),
                                                                                           SampleDistance ))
            .Invoke( Rays.Dir,
                     Rays.MinDistance,
                     Rays.MaxDistance,
                     camera.FrameBuffer,
                     *ScalarField);
        }
     }

    if(DoCompositeBackground)
    {
      vtkm::worklet::DispatcherMapField< CompositeBackground >( CompositeBackground( BackgroundColor ) )
        .Invoke( camera.FrameBuffer );
    }
    camera.WriteToSurface(canvas, Rays.MinDistance);
  } //Render

  VTKM_CONT
  void SetSampleDistance(const vtkm::Float32 & distance)
  {
    if(distance <= 0.f)
        throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
    SampleDistance = distance;
  }

  VTKM_CONT
  void SetBackgroundColor(const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
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
  VolumeRay<DeviceAdapter> Rays;
  Camera<DeviceAdapter> camera;
  vtkm::cont::DynamicArrayHandleCoordinateSystem Coordinates;
  vtkm::cont::CellSetStructured<3> Cellset;
  const vtkm::cont::Field *ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
  vtkm::Float32 SampleDistance;
  vtkm::Vec<vtkm::Float32,4> BackgroundColor;
  vtkm::Range ScalarRange;
  vtkm::Bounds DataBounds;

};
}}} //namespace vtkm::rendering::raytracing
#endif
