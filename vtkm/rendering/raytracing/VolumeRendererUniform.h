//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_VolumeRendererUniform_h
#define vtk_m_rendering_raytracing_VolumeRendererUniform_h

#include <math.h>
#include <iostream>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ErrorControlBadValue.h>
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
class VolumeRendererUniform
{
public:

  class Sampler : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;
    //vtkm::Float32 BoundingBox[6];
    vtkm::Float32 SampleDistance;
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Vec<vtkm::Float32,3> Origin;
    vtkm::Vec<vtkm::Float32,3> InvSpacing;
    vtkm::Id3 PointDimensions;
    vtkm::Float32 MinScalar;
    vtkm::Float32 InverseDeltaScalar;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    UniformConstPortal Coordinates;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,3> Conn;
  public:
    VTKM_CONT_EXPORT
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
      std::cout<<"Spacing "<<spacing<<std::endl;
      InvSpacing[0] = 1.f / spacing[0];
      InvSpacing[1] = 1.f / spacing[1];
      InvSpacing[2] = 1.f / spacing[2];
      std::cout<<"Max s "<<maxScalar<<" min s "<<minScalar<<std::endl;
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
    VTKM_EXEC_EXPORT
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
      //TODO: Just do this manually, this just does unneeded calcs
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
    VTKM_EXEC_EXPORT
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
      vtkm::Vec<vtkm::Float32,3> bottomLeft;
      bool newCell = true;
      //check to see if we left the cell
      //vtkm::Float32 deltaTx = SampleDistance * rayDir[0];
      //vtkm::Float32 deltaTx = SampleDistance * rayDir[0];
      //vtkm::Float32 deltaTx = SampleDistance * rayDir[0];
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
        std::cout.precision(10);

        //std::cout<<sampleLocation<<" current dist "<<currentDistance<<" max "<<lastSample<<" "<<vtkm::Float32(PointDimensions[0] - 1)<<std::endl;
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

        vtkm::Id colorIndex = static_cast<vtkm::Id>(finalScalar * static_cast<vtkm::Float32>(ColorMapSize));
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
    vtkm::Float32 SampleDistance;
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Vec<vtkm::Float32,3> Origin;
    vtkm::Vec<vtkm::Float32,3> InvSpacing;
    vtkm::Id3 CellDimensions;
    vtkm::Float32 MinScalar;
    vtkm::Float32 InverseDeltaScalar;
    ColorArrayPortal ColorMap;
    vtkm::Id ColorMapSize;
    UniformConstPortal Coordinates;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,3> Conn;
  public:
    VTKM_CONT_EXPORT
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
    VTKM_EXEC_EXPORT
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
    VTKM_EXEC_EXPORT
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
      vtkm::Float32 deltaTx = SampleDistance * rayDir[0];
      vtkm::Float32 deltaTy = SampleDistance * rayDir[1];
      vtkm::Float32 deltaTz = SampleDistance * rayDir[2];
      vtkm::Float32 tx = 2.f;
      vtkm::Float32 ty = 2.f;
      vtkm::Float32 tz = 2.f;
      vtkm::Float32 scalar0 = 0.f;
      vtkm::Vec<vtkm::Float32,4> sampleColor;
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
          scalar0 = (scalar0 - MinScalar) * InverseDeltaScalar;
          sampleColor = ColorMap.Get(static_cast<vtkm::Id>(scalar0 * static_cast<vtkm::Float32>(ColorMapSize)));
          sampleColor[3] = .05f;
          vtkm::Vec<vtkm::Float32,3> bottomLeft = Coordinates.Get(cellId);
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

        //std::cout<<" current color "<<color;
        if(color[3] >= 1.f) break;


        tx += deltaTx;
        ty += deltaTy;
        tz += deltaTz;
      }
      //color[0] = vtkm::Min(color[0],1.f);
      //color[1] = vtkm::Min(color[1],1.f);
      //color[2] = vtkm::Min(color[2],1.f);
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
   vtkm::Vec<vtkm::Float32,3> CameraPosition;
  public:
    VTKM_CONT_EXPORT
    CalcRayStart(vtkm::Vec<vtkm::Float32,3> cameraPosition,
                 vtkm::Float32 boundingBox[6])
     : CameraPosition(cameraPosition)
    {
      Xmin = boundingBox[0];
      Xmax = boundingBox[1];
      Ymin = boundingBox[2];
      Ymax = boundingBox[3];
      Zmin = boundingBox[4];
      Zmax = boundingBox[5];
    }

    VTKM_EXEC_EXPORT
    vtkm::Float32 rcp(vtkm::Float32 f)  const { return 1.0f/f;}

    VTKM_EXEC_EXPORT
    vtkm::Float32 rcp_safe(vtkm::Float32 f) const { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }

    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3);
    VTKM_EXEC_EXPORT
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
    VTKM_CONT_EXPORT
    CompositeBackground(const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
     : BackgroundColor(backgroundColor)
    {}

    typedef void ControlSignature(FieldInOut<>);
    typedef void ExecutionSignature(_1);
    VTKM_EXEC_EXPORT
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

  VTKM_CONT_EXPORT
  VolumeRendererUniform()
  {
    IsSceneDirty = false;
    IsUniformDataSet = true;
    NumberOfSamples = 300;
  }

  VTKM_CONT_EXPORT
  Camera<DeviceAdapter>& GetCamera()
  {
    return camera;
  }

  VTKM_CONT_EXPORT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colorMap)
  {
    ColorMap = colorMap;
  }

  VTKM_CONT_EXPORT
  void Init()
  {
    camera.CreateRays(Rays);
    IsSceneDirty = true;
  }

  VTKM_CONT_EXPORT
  void SetData(const vtkm::cont::ArrayHandleUniformPointCoordinates &coordinates,
               vtkm::cont::Field &scalarField,
               vtkm::Float64 coordsBounds[6],
               const vtkm::cont::CellSetStructured<3> &cellset,
               vtkm::Float64 *scalarBounds)
  {
    IsSceneDirty = true;
    Coordinates = coordinates;
    ScalarField = &scalarField;
    Cellset = cellset;
    ScalarBounds = scalarBounds;
    for (int i = 0; i < 6; ++i)
    {
      BoundingBox[i] = vtkm::Float32(coordsBounds[i]);
    }
  }


  VTKM_CONT_EXPORT
  void Render()
  {
    if(IsSceneDirty)
    {
      Init();
    }
    vtkm::Float32 sampleDistance = 1.f;
    vtkm::Vec<vtkm::Float32,3> extent;
    extent[0] = BoundingBox[1] - BoundingBox[0];
    extent[1] = BoundingBox[3] - BoundingBox[2];
    extent[2] = BoundingBox[5] - BoundingBox[4];
    sampleDistance = vtkm::Magnitude(extent) / vtkm::Float32(NumberOfSamples);
    std::cout<<"SampleDistance "<<sampleDistance<<std::endl;
    //Clear the framebuffer
    RGBA.Allocate(camera.GetWidth() * camera.GetHeight());
    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Vec<vtkm::Float32,4> > >( MemSet< vtkm::Vec<vtkm::Float32,4> >( BackgroundColor ) )
      .Invoke( RGBA );

    vtkm::worklet::DispatcherMapField< CalcRayStart >( CalcRayStart( camera.GetPosition(), BoundingBox ))
      .Invoke( Rays.Dir,
               Rays.MinDistance,
               Rays.MaxDistance);

    bool isSupportedField = (ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS ||
                             ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET );
    if(!isSupportedField) throw vtkm::cont::ErrorControlBadValue("Feild not accociated with cell set or points");
    bool isAssocPoints = ScalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS;

    if(isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField< Sampler >( Sampler( camera.GetPosition(),
                                                             ColorMap,
                                                             Coordinates,
                                                             Cellset,
                                                             vtkm::Float32(ScalarBounds[0]),
                                                             vtkm::Float32(ScalarBounds[1]),
                                                             sampleDistance ))
        .Invoke( Rays.Dir,
                 Rays.MinDistance,
                 Rays.MaxDistance,
                 RGBA,
                 ScalarField->GetData());
    }
    else
    {
      vtkm::worklet::DispatcherMapField< SamplerCellAssoc >( SamplerCellAssoc( camera.GetPosition(),
                                                                               ColorMap,
                                                                               Coordinates,
                                                                               Cellset,
                                                                               vtkm::Float32(ScalarBounds[0]),
                                                                               vtkm::Float32(ScalarBounds[1]),
                                                                               sampleDistance ))
        .Invoke( Rays.Dir,
                 Rays.MinDistance,
                 Rays.MaxDistance,
                 RGBA,
                 ScalarField->GetData());
    }


  vtkm::worklet::DispatcherMapField< CompositeBackground >( CompositeBackground( BackgroundColor ) )
      .Invoke( RGBA );
  } //Render

  VTKM_CONT_EXPORT
  void SetNumberOfSamples(vtkm::Int32 numSamples)
  {
    if(numSamples > 0) NumberOfSamples = numSamples;
  }

  VTKM_CONT_EXPORT
  void SetBackgroundColor(const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
  {
    BackgroundColor = backgroundColor;
  }

protected:
  bool IsSceneDirty;
  bool IsUniformDataSet;
  VolumeRay<DeviceAdapter> Rays;
  Camera<DeviceAdapter> camera;
  vtkm::cont::ArrayHandleUniformPointCoordinates Coordinates;
  vtkm::cont::CellSetStructured<3> Cellset;
  vtkm::cont::Field *ScalarField;
  ColorBuffer4f RGBA;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
  vtkm::Float32 BoundingBox[6];
  vtkm::Float32 NumberOfSamples;
  vtkm::Vec<vtkm::Float32,4> BackgroundColor;
  vtkm::Float64 *ScalarBounds;

};
}}} //namespace vtkm::rendering::raytracing
#endif
