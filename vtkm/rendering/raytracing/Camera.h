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
#ifndef vtk_m_rendering_raytracing_Camera_h
#define vtk_m_rendering_raytracing_Camera_h
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>
#include <vtkm/rendering/RenderSurfaceRayTracer.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <limits>

namespace vtkm {
namespace rendering {
namespace raytracing {
template< typename DeviceAdapter >
class Camera
{
public:

  class SurfaceConverter : public vtkm::worklet::WorkletMapField
  {
    vtkm::Float32 Proj22;
    vtkm::Float32 Proj23;
    vtkm::Float32 Proj32;
    vtkm::Int32 Width;
    vtkm::Int32 SubsetWidth;
    vtkm::Int32 Xmin;
    vtkm::Int32 Ymin;
    vtkm::Int32 NumPixels;

  public:
    VTKM_CONT_EXPORT
    SurfaceConverter(const vtkm::Int32 &width,
                     const vtkm::Int32 &subsetWidth,
                     const vtkm::Int32 &xmin,
                     const vtkm::Int32 &ymin,
                     const vtkm::Matrix<vtkm::Float32,4,4> projMat,
                     const vtkm::Int32 &numPixels)
    {
      Width = width;
      SubsetWidth = subsetWidth;
      Xmin = xmin;
      Ymin = ymin;
      Proj22 = projMat[2][2];
      Proj23 = projMat[2][3];
      Proj32 = projMat[3][2];
      NumPixels = numPixels;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  ExecObject,
                                  ExecObject);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    _4,
                                    WorkIndex);
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Vec<vtkm::Float32,4> &inColor,
                    const vtkm::Float32 &inDepth,
                    vtkm::exec::ExecutionWholeArray<vtkm::Float32> &depthBuffer,
                    vtkm::exec::ExecutionWholeArray<vtkm::Float32> &colorBuffer,
                    const vtkm::Id &index) const
    {
      if(index >=  NumPixels) return;
      vtkm::Float32 depth = (Proj22 + Proj23 / (-inDepth)) / Proj32;
      depth = 0.5f * depth + 0.5f;
      vtkm::Int32 x = index % SubsetWidth;
      vtkm::Int32 y = index / SubsetWidth;
      x += Xmin;
      y += Ymin;
      vtkm::Id outIdx = vtkm::Id(y * Width + x);
      //std::cout<<" "<<depth;
      depthBuffer.Set(outIdx, depth);

      outIdx = outIdx * 4;
      colorBuffer.Set(outIdx + 0, inColor[0]);
      colorBuffer.Set(outIdx + 1, inColor[1]);
      colorBuffer.Set(outIdx + 2, inColor[2]);
      colorBuffer.Set(outIdx + 3, inColor[3]);
    }
  }; //class SurfaceConverter

  class PerspectiveRayGen : public vtkm::worklet::WorkletMapField
  {
  public:
    vtkm::Int32 w;
    vtkm::Int32 h;
    vtkm::Int32 Minx;
    vtkm::Int32 Miny;
    vtkm::Int32 SubsetWidth;
    vtkm::Vec< vtkm::Float32, 3> nlook;// normalized look
    vtkm::Vec< vtkm::Float32, 3> delta_x;
    vtkm::Vec< vtkm::Float32, 3> delta_y;
    VTKM_CONT_EXPORT
    PerspectiveRayGen(vtkm::Int32   width,
                      vtkm::Int32   height,
                      vtkm::Float32 fovX,
                      vtkm::Float32 fovY,
                      vtkm::Vec< vtkm::Float32, 3> look,
                      vtkm::Vec< vtkm::Float32, 3> up,
                      vtkm::Float32 _zoom,
                      vtkm::Int32 subsetWidth,
                      vtkm::Int32 minx,
                      vtkm::Int32 miny)
    : w(width),
      h(height),
      SubsetWidth(subsetWidth),
      Minx(minx),
      Miny(miny)
    {
      vtkm::Float32 thx = tanf( (fovX*vtkm::Float32(vtkm::Pi())/180.f) *.5f);
      vtkm::Float32 thy = tanf( (fovY*vtkm::Float32(vtkm::Pi())/180.f) *.5f);
      vtkm::Vec< vtkm::Float32, 3> ru = vtkm::Cross(look,up);
      vtkm::Normalize(ru);

      vtkm::Vec< vtkm::Float32, 3> rv = vtkm::Cross(ru,look);
      vtkm::Normalize(rv);
      delta_x = ru*(2*thx/(float)w);
      delta_y = rv*(2*thy/(float)h);

      if(_zoom > 0)
      {
          delta_x[0] = delta_x[0] / _zoom;
          delta_x[1] = delta_x[1] / _zoom;
          delta_x[2] = delta_x[2] / _zoom;
          delta_y[0] = delta_y[0] / _zoom;
          delta_y[1] = delta_y[1] / _zoom;
          delta_y[2] = delta_y[2] / _zoom;
      }
      nlook = look;
      vtkm::Normalize(nlook);

    }

    typedef void ControlSignature(FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<> );

    typedef void ExecutionSignature(WorkIndex,
                                    _1,
                                    _2,
                                    _3);
    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id idx,
                    vtkm::Float32 &rayDirX,
                    vtkm::Float32 &rayDirY,
                    vtkm::Float32 &rayDirZ) const
    {
      vtkm::Vec<vtkm::Float32,3> ray_dir(rayDirX, rayDirY, rayDirZ);
      int i = vtkm::Int32( idx ) % SubsetWidth;
      int j = vtkm::Int32( idx ) / SubsetWidth;
      i += Minx;
      j += Miny;
      ray_dir = nlook + delta_x * ((2.f * vtkm::Float32(i) - vtkm::Float32(w)) / 2.0f)
                      + delta_y * ((2.f * vtkm::Float32(j) - vtkm::Float32(h)) / 2.0f);
      vtkm::Normalize(ray_dir);
      rayDirX = ray_dir[0];
      rayDirY = ray_dir[1];
      rayDirZ = ray_dir[2];
    }

  };// class perspective ray gen
private:
  vtkm::Int32   Height;
  vtkm::Int32   Width;
  vtkm::Int32   SubsetWidth;
  vtkm::Int32   SubsetHeight;
  vtkm::Int32   SubsetMinX;
  vtkm::Int32   SubsetMinY;
  vtkm::Float32 FovX;
  vtkm::Float32 FovY;
  vtkm::Float32 FOV;
  vtkm::Float32 Zoom;
  bool        IsViewDirty;
  bool        IsResDirty;
  bool        MortonSort;
  bool        LookAtSet;
  //bool        ImageSubsetModeOn;

  vtkm::Vec< vtkm::Float32, 3> Look;
  vtkm::Vec< vtkm::Float32, 3> Up;
  vtkm::Vec< vtkm::Float32, 3> LookAt;
  vtkm::Vec< vtkm::Float32, 3> Position;
  View CameraView;
  vtkm::Matrix<vtkm::Float32,4,4> ViewProjectionMat;


public:

  ColorBuffer4f FrameBuffer;

  VTKM_CONT_EXPORT
  Camera()
  {
    this->Height = 500;
    this->Width = 500;
    this->SubsetWidth = 500;
    this->SubsetHeight = 500;
    this->SubsetMinX = 0;
    this->SubsetMinY = 0;
    this->FovY = 30.f;
    this->FovX = 30.f;
    this->Zoom = 1.f;
    this->Look[0] = 0.f;
    this->Look[1] = 0.f;
    this->Look[2] = -1.f;
    this->LookAt[0] = 0.f;
    this->LookAt[1] = 0.f;
    this->LookAt[2] = -1.f;
    this->Up[0] = 0.f;
    this->Up[1] = 1.f;
    this->Up[2] = 0.f;
    this->Position[0] = 0.f;
    this->Position[1] = 0.f;
    this->Position[2] = 0.f;
    this->IsViewDirty = true;
    this->IsResDirty = true;
    this->MortonSort = false;
    this->FrameBuffer.Allocate(Height * Width);
    //this->ImageSubsetModeOn = true;
  }

  VTKM_CONT_EXPORT
  void SetParameters(View &view)
  {
    this->SetUp(view.View3d.Up);
    this->SetLookAt(view.View3d.LookAt);
    this->SetPosition(view.View3d.Position);
    this->SetFieldOfView(view.View3d.FieldOfView);
    this->SetHeight(view.Height);
    this->SetWidth(view.Width);
    this->CameraView = view;
  }


  VTKM_CONT_EXPORT
  void SetHeight(const vtkm::Int32 &height)
  {
    if(height <= 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Camera height must be greater than zero.");
    }
    if(Height != height)
    {
      this->IsResDirty = true;
      this->Height = height;
      this->SetFieldOfView(this->FovX);
      this->CameraView.Height = this->Height;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Int32 GetHeight() const
  {
    return this->Height;
  }

  VTKM_CONT_EXPORT
  void SetWidth(const vtkm::Int32 &width)
  {
    if(width <= 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Camera width must be greater than zero.");
    }
    if(this->Width != width)
    {
      this->IsResDirty = true;
      this->Width = width;
      this->SetFieldOfView(this->FovX);
      this->CameraView.Width = this->Width;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Int32 GetWidth() const
  {
    return this->Width;
  }

  VTKM_CONT_EXPORT
  void SetZoom(const vtkm::Float32 &zoom)
  {
    if(zoom <= 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Camera zoom must be greater than zero.");
    }
    if(this->Zoom != zoom)
    {
      this->IsViewDirty = true;
      this->Zoom = zoom;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Float32 GetZoom() const
  {
    return this->Zoom;
  }

  VTKM_CONT_EXPORT
  void SetFieldOfView(const vtkm::Float32 &degrees)
  {
    if(degrees <= 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Camera feild of view must be greater than zero.");
    }
    if(degrees > 180)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Camera feild of view must be less than 180.");
    }
    // fov is stored as a half angle
    // float fovx= 2.f*atan(tan(view.view3d.fov/2.f)*view.w/view.h);
    // fovx*=180.f/M_PI;
    // camera->setFOVY((view.view3d.fov*(180.f/M_PI))/2.f);
    // camera->setFOVX( fovx/2.f );

    vtkm::Float32 newFOVY =
        (vtkm::Float32(this->Height) / vtkm::Float32(this->Width)) * degrees;
    vtkm::Float32 newFOVX = degrees;
    if(newFOVX != this->FovX) { this->IsViewDirty = true; }
    if(newFOVY != this->FovY) { this->IsViewDirty = true; }
    this->FovX = newFOVX;
    this->FovY = newFOVY;
    this->CameraView.View3d.FieldOfView = this->FovX;
  }

  VTKM_CONT_EXPORT
  vtkm::Float32 GetFieldOfView() const
  {
    return this->FovX;
  }

  VTKM_CONT_EXPORT
  void SetUp(const vtkm::Vec<vtkm::Float32, 3> &up)
  {
    if(this->Up != up)
    {
      this->Up = up;
      vtkm::Normalize(this->Up);
      this->IsViewDirty = true;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3> GetUp() const
  {
    return this->Up;
  }

  VTKM_CONT_EXPORT
  void SetLookAt(const vtkm::Vec<vtkm::Float32, 3> &lookAt)
  {
    if(this->LookAt != lookAt)
    {
      this->LookAt = lookAt;
      this->IsViewDirty = true;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3> GetLookAt() const
  {
    return this->LookAt;
  }

  VTKM_CONT_EXPORT
  void SetPosition(const vtkm::Vec<vtkm::Float32, 3> &position)
  {
    if(this->Position != position)
    {
      this->Position = position;
      this->IsViewDirty = true;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3> GetPosition() const
  {
    return this->Position;
  }

  VTKM_CONT_EXPORT
  void ResetIsViewDirty()
  {
    this->IsViewDirty = false;
  }

  VTKM_CONT_EXPORT
  bool GetIsViewDirty() const
  {
    return this->IsViewDirty;
  }

  VTKM_CONT_EXPORT
  void WriteToSurface(RenderSurfaceRayTracer *surface,
                      const vtkm::cont::ArrayHandle<vtkm::Float32> &distances)
  {
    if(surface == NULL)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Camera can not write to NULL surface");
    }
    if(this->Height != vtkm::Int32(surface->Height) ||
       this->Width != vtkm::Int32(surface->Width))
    {
      throw vtkm::cont::ErrorControlBadValue("Camera: suface-view mismatched dims");
    }
    vtkm::worklet::DispatcherMapField< SurfaceConverter >(
          SurfaceConverter( this->Width,
                            this->SubsetWidth,
                            this->SubsetMinX,
                            this->SubsetMinY,
                            this->CameraView.CreateProjectionMatrix(),
                            this->SubsetWidth * this->SubsetHeight) )
        .Invoke( this->FrameBuffer,
                 distances,
                 vtkm::exec::ExecutionWholeArray<vtkm::Float32>(surface->DepthArray),
                 vtkm::exec::ExecutionWholeArray<vtkm::Float32>(surface->ColorArray) );

    //Force the transfer so the vectors contain data from device
    surface->ColorArray.GetPortalControl().Get(0);
    surface->DepthArray.GetPortalControl().Get(0);
  }

  VTKM_CONT_EXPORT
  void CreateRays(Ray<DeviceAdapter> &rays,
                  vtkm::Float64 *boundingBox = NULL)
  {
    this->UpdateDimensions(&rays, boundingBox);
    //Set the origin of the ray back to the camera position
    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32>( this->Position[0] ) )
      .Invoke( rays.OriginX );

    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32>( this->Position[1] ) )
      .Invoke( rays.OriginY );

    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32>( this->Position[2] ) )
      .Invoke( rays.OriginZ );

    //Reset the camera look vector
    this->Look = this->LookAt - this->Position;
    vtkm::Normalize(this->Look);
    //Create the ray direction
    vtkm::worklet::DispatcherMapField< PerspectiveRayGen >(
          PerspectiveRayGen(this->Width,
                            this->Height,
                            this->FovX,
                            this->FovY,
                            this->Look,
                            this->Up,
                            this->Zoom,
                            this->SubsetWidth,
                            this->SubsetMinX,
                            this->SubsetMinY) )
      .Invoke(rays.DirX,
              rays.DirY,
              rays.DirZ); //X Y Z

    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32 >( 1e12f ) )
      .Invoke( rays.Distance );

    //Reset the Rays Hit Index to -2
    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Id > >( MemSet< vtkm::Id >( -2 ) )
      .Invoke( rays.HitIdx );

  } //create rays

    VTKM_CONT_EXPORT
  void CreateRays(VolumeRay<DeviceAdapter> &rays,
                  vtkm::Float64 *boundingBox = NULL)
  {

    this->UpdateDimensions(&rays, boundingBox);

    //Reset the camera look vector
    this->Look = this->LookAt - this->Position;
    vtkm::Normalize(this->Look);
    //Create the ray direction
    vtkm::worklet::DispatcherMapField< PerspectiveRayGen >(
          PerspectiveRayGen(this->Width,
                            this->Height,
                            this->FovX,
                            this->FovY,
                            this->Look,
                            this->Up,
                            this->Zoom,
                            this->SubsetWidth,
                            this->SubsetMinX,
                            this->SubsetMinY) )
      .Invoke(rays.DirX,
              rays.DirY,
              rays.DirZ); //X Y Z

  } //create rays

private:
  VTKM_CONT_EXPORT
  void FindSubset(vtkm::Float64 *bounds)
  {
    vtkm::Float32 x[2], y[2], z[2];
    x[0] = static_cast<vtkm::Float32>(bounds[0]);
    x[1] = static_cast<vtkm::Float32>(bounds[1]);
    y[0] = static_cast<vtkm::Float32>(bounds[2]);
    y[1] = static_cast<vtkm::Float32>(bounds[3]);
    z[0] = static_cast<vtkm::Float32>(bounds[4]);
    z[1] = static_cast<vtkm::Float32>(bounds[5]);
    //Inise the data bounds
    if(this->Position[0] >=x[0] && this->Position[0] <=x[1] &&
       this->Position[1] >=y[0] && this->Position[1] <=y[1] &&
       this->Position[2] >=z[0] && this->Position[2] <=z[1] )
    {
      this->SubsetWidth = this->Width;
      this->SubsetHeight = this->Height;
      this->SubsetMinY = 0;
      this->SubsetMinX = 0;
      return;
    }

    //std::cout<<"Bounds ("<<x[0]<<","<<y[0]<<","<<z[0]<<")-("<<x[1]<<","<<y[1]<<","<<z[1]<<std::endl;
    vtkm::Float32 xmin, ymin, xmax, ymax, zmin, zmax;
    xmin = std::numeric_limits<vtkm::Float32>::max();
    ymin = std::numeric_limits<vtkm::Float32>::max();
    zmin = std::numeric_limits<vtkm::Float32>::max();
    xmax = std::numeric_limits<vtkm::Float32>::min();
    ymax = std::numeric_limits<vtkm::Float32>::min();
    zmax = std::numeric_limits<vtkm::Float32>::min();
    vtkm::Vec<vtkm::Float32,4> extentPoint;
    for (vtkm::Int32 i = 0; i < 2; ++i)
    for (vtkm::Int32 j = 0; j < 2; ++j)
    for (vtkm::Int32 k = 0; k < 2; ++k)
    {
      extentPoint[0] = x[i];
      extentPoint[1] = y[j];
      extentPoint[2] = z[k];
      extentPoint[3] = 1.f;
      vtkm::Vec<vtkm::Float32,4> transformed =
          vtkm::MatrixMultiply(this->ViewProjectionMat,extentPoint);
      // perform the perspective divide
      for (vtkm::Int32 a = 0; a < 3; ++a)
      {
        transformed[a] = transformed[a] / transformed[3];
      }

      transformed[0] = (transformed[0] * 0.5f + 0.5f) * Width;
      transformed[1] = (transformed[1] * 0.5f + 0.5f) * Height;
      transformed[2] = (transformed[2] * 0.5f + 0.5f);
      zmin = vtkm::Min(zmin, transformed[2]);
      zmax = vtkm::Max(zmax, transformed[2]);
      if(transformed[2] < 0 || transformed[2] > 1) { continue; }
      xmin = vtkm::Min(xmin, transformed[0]);
      ymin = vtkm::Min(ymin, transformed[1]);
      xmax = vtkm::Max(xmax, transformed[0]);
      ymax = vtkm::Max(ymax, transformed[1]);
    }

    xmin -= .001f;
    xmax += .001f;
    ymin -= .001f;
    ymax += .001f;
    xmin = floor(vtkm::Min(vtkm::Max(0.f, xmin),vtkm::Float32(this->Width) ));
    xmax =  ceil(vtkm::Min(vtkm::Max(0.f, xmax),vtkm::Float32(this->Width) ));
    ymin = floor(vtkm::Min(vtkm::Max(0.f, ymin),vtkm::Float32(this->Height) ));
    ymax =  ceil(vtkm::Min(vtkm::Max(0.f, ymax),vtkm::Float32(this->Height) ));
    //printf("Pixel range = (%f,%f,%f), (%f,%f,%f)\n", xmin, ymin,zmin, xmax,ymax,zmax);
    vtkm::Int32 dx = vtkm::Int32(xmax) - vtkm::Int32(xmin);
    vtkm::Int32 dy = vtkm::Int32(ymax) - vtkm::Int32(ymin);

    //
    //  scene is behind the camera
    //
    if(zmax < 0 || zmin > 1 ||
       xmin >= xmax ||
       ymin >= ymax)
    {
      this->SubsetWidth = 1;
      this->SubsetHeight = 1;
      this->SubsetMinX = 0;
      this->SubsetMinY = 0;
    }
    else
    {
      this->SubsetWidth  = dx;
      this->SubsetHeight = dy;
      this->SubsetMinX = vtkm::Int32(xmin);
      this->SubsetMinY = vtkm::Int32(ymin);
    }

  }

  VTKM_CONT_EXPORT
  void UpdateDimensions(RayBase *rays,
                        vtkm::Float64 *boundingBox = NULL)
  {
     // If bounds have been provided, only cast rays that could hit the data
    bool imageSubsetModeOn = boundingBox != NULL;

    //Update our ViewProjection matrix
    this->ViewProjectionMat
      = vtkm::MatrixMultiply(this->CameraView.CreateProjectionMatrix(),
                             this->CameraView.CreateViewMatrix());

    //Find the pixel footprint
    if(imageSubsetModeOn)
    {
      this->FindSubset(boundingBox);
    }

    //Update the image dimensions
    if(!imageSubsetModeOn)
    {
      this->SubsetWidth = this->Width;
      this->SubsetHeight = this->Height;
      this->SubsetMinY = 0;
      this->SubsetMinX = 0;
    }
    else
    {
      if(this->SubsetWidth != this->Width) { this->IsResDirty = true; }
      if(this->SubsetHeight != this->Height) { this->IsResDirty = true; }
    }
    // resize rays and buffers
    if(this->IsResDirty)
    {
      rays->resize(this->SubsetHeight * this->SubsetWidth);
      this->FrameBuffer.PrepareForOutput(this->SubsetHeight * this->SubsetWidth,
                                         DeviceAdapter());
    }

    this->IsResDirty = false;

  }

}; // class camera
}}}//namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Camera_h
