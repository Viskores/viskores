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
#ifndef vtk_m_rendering_raytracing_RayTracer_h
#define vtk_m_rendering_raytracing_RayTracer_h
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>


namespace vtkm {
namespace rendering {
namespace raytracing {

class IntersectionPoint : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT_EXPORT
  IntersectionPoint() {}
  typedef void ControlSignature(FieldIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>);
  typedef void ExecutionSignature(_1,
                                  _2,
                                  _3,
                                  _4,
                                  _5,
                                  _6,
                                  _7);
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Id &hitIndex,
                  const vtkm::Float32 &distance,
                  const vtkm::Vec<vtkm::Float32,3> &rayDir,
                  const vtkm::Vec<vtkm::Float32,3> &rayOrigin,
                  vtkm::Float32 &intersectionX,
                  vtkm::Float32 &intersectionY,
                  vtkm::Float32 &intersectionZ) const
  {
    if(hitIndex < 0) return;

    intersectionX = rayOrigin[0] + rayDir[0] * distance;
    intersectionY = rayOrigin[1] + rayDir[1] * distance;
    intersectionZ = rayOrigin[2] + rayDir[2] * distance;
  }
}; //class IntersectionPoint

template<typename DeviceAdapter>
class Reflector
{
public:
  // Worklet to calutate the normals of a triagle if
  // none are stored in the data set
  class CalculateNormals : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32,4> >  Vec4IntArrayHandle; 
    typedef typename Vec4IntArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IndicesArrayPortal;
    
    IndicesArrayPortal IndicesPortal;
  public:
    VTKM_CONT_EXPORT
    CalculateNormals(const Vec4IntArrayHandle &indices)
      : IndicesPortal( indices.PrepareForInput( DeviceAdapter() ) )
    {}
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  WholeArrayIn<Vec3RenderingTypes>);
    typedef void ExecutionSignature(_1, 
                                    _2,
                                    _3,
                                    _4,
                                    _5,
                                    _6);
    template<typename PointPortalType>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &hitIndex,
                    const vtkm::Vec<vtkm::Float32,3> &rayDir, 
                    vtkm::Float32 &normalX,
                    vtkm::Float32 &normalY,
                    vtkm::Float32 &normalZ,
                    const PointPortalType &points) const
    {
      if(hitIndex < 0) return;
      
      vtkm::Vec<Int32, 4> indices = IndicesPortal.Get(hitIndex);
      vtkm::Vec<Float32, 3> a = points.Get(indices[1]);
      vtkm::Vec<Float32, 3> b = points.Get(indices[2]);
      vtkm::Vec<Float32, 3> c = points.Get(indices[3]);

      vtkm::Vec<Float32, 3> normal = vtkm::TriangleNormal(a,b,c);
      vtkm::Normalize(normal);
      //flip the normal if its pointing the wrong way
      if(vtkm::dot(normal,rayDir) < 0.f) normal = -normal;
      normalX = normal[0];
      normalY = normal[1];
      normalZ = normal[2];
    }
  }; //class CalculateNormals

  class LerpScalar : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32,4> >  Vec4IntArrayHandle; 
    typedef typename Vec4IntArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IndicesArrayPortal;

    IndicesArrayPortal IndicesPortal;
    vtkm::Float32 MinScalar;
    vtkm::Float32 invDeltaScalar;
  public:
   
    VTKM_CONT_EXPORT
    LerpScalar(const Vec4IntArrayHandle &indices,
                 const vtkm::Float32 &minScalar,
                 const vtkm::Float32 &maxScalar)
      : IndicesPortal( indices.PrepareForInput( DeviceAdapter() ) ),
        MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on 
      //something like an iso-surface
      if(maxScalar - MinScalar != 0.f) invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else invDeltaScalar = 1.f / minScalar;
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
    template<typename ScalarPortalType>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &hitIndex, 
                    const vtkm::Float32 &u,
                    const vtkm::Float32 &v,
                    vtkm::Float32 &lerpedScalar,
                    const ScalarPortalType &scalars) const
    {
      if(hitIndex < 0) return;
      
      vtkm::Vec<Int32, 4> indices = IndicesPortal.Get(hitIndex);
      
      vtkm::Float32 n = 1.f - u - v;
      vtkm::Float32 aScalar = vtkm::Float32(scalars.Get(indices[1]));
      vtkm::Float32 bScalar = vtkm::Float32(scalars.Get(indices[2]));
      vtkm::Float32 cScalar = vtkm::Float32(scalars.Get(indices[3]));
      lerpedScalar = aScalar * n  + bScalar * u + cScalar * v;
      //normalize
      lerpedScalar = (lerpedScalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar


  class NodalScalar : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32,4> >  Vec4IntArrayHandle; 
    typedef typename Vec4IntArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IndicesArrayPortal;

    IndicesArrayPortal IndicesPortal;
    vtkm::Float32 MinScalar;
    vtkm::Float32 invDeltaScalar;
  public:
   
    VTKM_CONT_EXPORT
    NodalScalar(const Vec4IntArrayHandle &indices,
                const vtkm::Float32 &minScalar,
                const vtkm::Float32 &maxScalar)
      : IndicesPortal( indices.PrepareForInput( DeviceAdapter() ) ),
        MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on 
      //something like an iso-surface
      if(maxScalar - MinScalar != 0.f) invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else invDeltaScalar = 1.f / minScalar;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>);
    typedef void ExecutionSignature(_1, 
                                    _2,
                                    _3);
    template<typename ScalarPortalType>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &hitIndex,
                    vtkm::Float32 &scalar,
                    const ScalarPortalType &scalars) const
    {
      if(hitIndex < 0) return;
      
      vtkm::Vec<Int32, 4> indices = IndicesPortal.Get(hitIndex);
      
      //Todo: one normalization
      scalar = vtkm::Float32(scalars.Get(indices[0]));
      
      //normalize
      scalar = (scalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar

  VTKM_CONT_EXPORT
  void run(Ray<DeviceAdapter> &rays, 
           LinearBVH &bvh, 
           vtkm::cont::DynamicArrayHandleCoordinateSystem &coordsHandle,
           vtkm::cont::Field *scalarField,
           vtkm::Float64 *scalarBounds)
  {
    bool isSupportedField = (scalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS || 
                             scalarField->GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET );
    if(!isSupportedField) throw vtkm::cont::ErrorControlBadValue("Feild not accociated with cell set or points");
    bool isAssocPoints = scalarField->GetAssociation() == vtkm::cont::Field::ASSOC_POINTS;
      
    vtkm::worklet::DispatcherMapField< CalculateNormals >( CalculateNormals(bvh.LeafNodes) )
      .Invoke(rays.HitIdx,
              rays.Dir,
              rays.NormalX,
              rays.NormalY,
              rays.NormalZ,
              coordsHandle);
    
    if(isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField< LerpScalar >( LerpScalar(bvh.LeafNodes, 
                                                               vtkm::Float32(scalarBounds[0]),
                                                               vtkm::Float32(scalarBounds[1])) )
        .Invoke(rays.HitIdx,
                rays.U,
                rays.V,
                rays.Scalar,
                scalarField->GetData()); 
    }
    else
    {
      vtkm::worklet::DispatcherMapField< NodalScalar >( NodalScalar(bvh.LeafNodes, 
                                                                    vtkm::Float32(scalarBounds[0]),
                                                                    vtkm::Float32(scalarBounds[1])) )
        .Invoke(rays.HitIdx,
                rays.Scalar,
                scalarField->GetData()); 
    } 
  } // Run
  
}; // Class reflector

template< typename DeviceAdapter>
class SurfaceColor
{ 
public:
  class MapScalarToColor : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
    typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;
  
    ColorArrayPortal ColorMap;
    vtkm::Int32 ColorMapSize;
    vtkm::Vec<vtkm::Float32,3> LightPosition;
    vtkm::Vec<vtkm::Float32,3> LightAbmient;
    vtkm::Vec<vtkm::Float32,3> LightDiffuse;
    vtkm::Vec<vtkm::Float32,3> LightSpecular;
    vtkm::Float32 SpecularExponent;
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
    vtkm::Vec<vtkm::Float32,4> BackgroundColor;
  public:
    
    VTKM_CONT_EXPORT
    MapScalarToColor(const ColorArrayHandle &colorMap,
                     const vtkm::Int32 &colorMapSize,
                     const vtkm::Vec<vtkm::Float32,3> &lightPosition,
                     const vtkm::Vec<vtkm::Float32,3> &cameraPosition,
                     const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
      : ColorMap( colorMap.PrepareForInput( DeviceAdapter() ) ),
        ColorMapSize(colorMapSize),
        LightPosition(lightPosition),
        CameraPosition(cameraPosition),
        BackgroundColor(backgroundColor)
    {
      //Set up some default lighting parameters for now
      LightAbmient[0] = .3f;
      LightAbmient[1] = .3f;
      LightAbmient[2] = .3f;
      LightDiffuse[0] = .7f;
      LightDiffuse[1] = .7f;
      LightDiffuse[2] = .7f;
      LightSpecular[0] = .7f;
      LightSpecular[1] = .7f;
      LightSpecular[2] = .7f;
      SpecularExponent = 80.f;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1, 
                                    _2,
                                    _3,
                                    _4,
                                    _5);
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &hitIdx,
                    const vtkm::Float32 &scalar,
                    const vtkm::Vec<vtkm::Float32,3> &normal,
                    const vtkm::Vec<vtkm::Float32,3> &intersection, 
                    vtkm::Vec<vtkm::Float32,4> &color) const
    {
      if(hitIdx < 0) {color = BackgroundColor; return;}
      vtkm::Vec<vtkm::Float32,3> lightDir = LightPosition - intersection;
      vtkm::Vec<vtkm::Float32,3> viewDir = CameraPosition - intersection;
      vtkm::Normalize(lightDir);
      vtkm::Normalize(viewDir);

      //Diffuse lighting 
      vtkm::Float32 cosTheta = vtkm::dot(-normal,lightDir);
      //clamp tp [0,1]
      cosTheta = vtkm::Min(vtkm::Max(cosTheta, 0.f), 1.f);

      //Specular lighting 
      vtkm::Vec<vtkm::Float32,3> halfVector = viewDir + lightDir;
      vtkm::Normalize(halfVector);
      vtkm::Float32 cosPhi = vtkm::dot(-normal,halfVector);
      vtkm::Float32 specularConstant = vtkm::Float32(pow(fmaxf(cosPhi,0.f), SpecularExponent)); 

      vtkm::Int32 colorIdx = vtkm::Int32(scalar * vtkm::Float32(ColorMapSize - 1));
      
      //Just in case clamp the value to the valid range
      colorIdx = (colorIdx < 0) ? 0 : colorIdx;
      colorIdx = (colorIdx > ColorMapSize - 1) ? ColorMapSize - 1 : colorIdx;
      color = ColorMap.Get(colorIdx);
      //std::cout<<" Before "<<color<< " at index "<<colorIdx<<" ";
      //std::cout<<"CosTheta "<<cosTheta<<" CosPhi "<<cosPhi<<" | ";
      color[0] *= vtkm::Min(LightAbmient[0] + LightDiffuse[0] * cosTheta + LightSpecular[0] * specularConstant, 1.f);
      color[1] *= vtkm::Min(LightAbmient[1] + LightDiffuse[1] * cosTheta + LightSpecular[1] * specularConstant, 1.f);
      color[2] *= vtkm::Min(LightAbmient[2] + LightDiffuse[2] * cosTheta + LightSpecular[2] * specularConstant, 1.f);  
      //color[0] =1.f;
      //std::cout<<color;
    }
  }; //class MapScalarToColor
  
  VTKM_CONT_EXPORT
  void run(Ray<DeviceAdapter> &rays, 
           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colorMap, 
           ColorBuffer4f &colorBuffer,
           const vtkm::Vec<vtkm::Float32,3> cameraPosition,
           const vtkm::Vec<vtkm::Float32,4> backgroundColor)
  {
    vtkm::Vec<vtkm::Float32,3> lightPosition = cameraPosition;
    // lightPosition[0] = 0.f;
    // lightPosition[1] = 0.f;
    // lightPosition[2] = -10.f;
    const vtkm::Int32 colorMapSize = vtkm::Int32(colorMap.GetNumberOfValues());
    vtkm::worklet::DispatcherMapField< MapScalarToColor >( MapScalarToColor(colorMap, 
                                                                            colorMapSize,
                                                                            lightPosition,
                                                                            cameraPosition,
                                                                            backgroundColor))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            rays.Normal,
            rays.Intersection,
            colorBuffer); 
  }
};// class SurfaceColor

template<typename DeviceAdapter>
class RayTracer
{
protected:
  bool IsSceneDirty;
  Ray<DeviceAdapter> Rays;
  LinearBVHBuilder<DeviceAdapter> Builder;
  LinearBVH Bvh;
  Camera<DeviceAdapter> camera;
  vtkm::cont::DynamicArrayHandleCoordinateSystem CoordsHandle;
  vtkm::cont::Field *ScalarField;
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > Indices;
  vtkm::cont::ArrayHandle<vtkm::Float32> Scalars;
  vtkm::Id NumberOfTriangles;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
  vtkm::Float64 *ScalarBounds;
  vtkm::Float64 *DataBounds;
  vtkm::Vec<vtkm::Float32,4> BackgroundColor;
public:
  VTKM_CONT_EXPORT
  RayTracer()
  {
    IsSceneDirty = true;
  }

  VTKM_CONT_EXPORT
  void SetBackgroundColor(const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
  {
    BackgroundColor = backgroundColor;
  }

  VTKM_CONT_EXPORT
  Camera<DeviceAdapter>& GetCamera()
  {
    return camera;
  }

  VTKM_CONT_EXPORT
  void SetData(const vtkm::cont::DynamicArrayHandleCoordinateSystem &coordsHandle,
               const vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  &indices,
               vtkm::cont::Field &scalarField,
               vtkm::Id &numberOfTriangles,
               vtkm::Float64 *scalarBounds,
               vtkm::Float64 *dataBounds)
  {
    IsSceneDirty = true;
    CoordsHandle = coordsHandle;
    Indices = indices;
    ScalarField = &scalarField;
    NumberOfTriangles = numberOfTriangles;
    ScalarBounds = scalarBounds;
    DataBounds = dataBounds;
  }

  VTKM_CONT_EXPORT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colorMap)
  {
    ColorMap = colorMap;
  }

  VTKM_CONT_EXPORT
  void Init()
  {
    Builder.run(CoordsHandle, Indices, NumberOfTriangles, Bvh);
    camera.CreateRays(Rays, DataBounds);
    IsSceneDirty = false;
  }

  VTKM_CONT_EXPORT
  void Render(RenderSurfaceRayTracer *surface)
  { 
    vtkm::cont::Timer<DeviceAdapter> renderTimer;
    if(IsSceneDirty)
    {
      Init();
    }
    
    TriangleIntersector<DeviceAdapter> intersector;
    intersector.run(Rays, Bvh, CoordsHandle);
    Reflector<DeviceAdapter> reflector;
    reflector.run(Rays, Bvh, CoordsHandle, ScalarField, ScalarBounds);
    vtkm::worklet::DispatcherMapField< IntersectionPoint >( IntersectionPoint() )
      .Invoke( Rays.HitIdx,
               Rays.Distance,
               Rays.Dir,
               Rays.Origin,
               Rays.IntersectionX,
               Rays.IntersectionY,
               Rays.IntersectionZ );

    
    SurfaceColor<DeviceAdapter> surfaceColor;
    surfaceColor.run(Rays, 
                     ColorMap, 
                     camera.FrameBuffer, 
                     camera.GetPosition(),
                     BackgroundColor);

    
     camera.WriteToSurface(surface, Rays.Distance);

  }
};//class RayTracer
}}}// namespace vtkm::rendering::raytracing 
#endif //vtk_m_rendering_raytracing_RayTracer_h
