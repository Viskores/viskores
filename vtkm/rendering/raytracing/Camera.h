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
#include <vtkm/rendering/raytracing/Worklets.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
namespace vtkm {
namespace rendering {
namespace raytracing {
template< typename DeviceAdapter >
class Camera
{
public:
  class PerspectiveRayGen : public vtkm::worklet::WorkletMapField
  {
  public:
    vtkm::Int32 w;
    vtkm::Int32 h; 
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
                      vtkm::Float32 _zoom)
    : w(width), h(height)
    {
      vtkm::Float32 thx = tanf( (fovX*3.1415926f/180.f) *.5f);
      vtkm::Float32 thy = tanf( (fovY*3.1415926f/180.f) *.5f);
      std::cout<<"Tan fovx "<<thx<<std::endl;
      vtkm::Vec< vtkm::Float32, 3> ru = vtkm::Cross(up,look);
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
      std::cout<<"Up/look/right "<<ru<<nlook<<rv<<std::endl;

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
      int i = vtkm::Int32( idx ) % w;
      int j = vtkm::Int32( idx ) / w;

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
  vtkm::Float32 FovX;
  vtkm::Float32 FovY;
  vtkm::Float32 FOV;
  vtkm::Float32 Zoom;
  bool        IsViewDirty;
  bool        IsResDirty;
  bool        MortonSort;
  bool        LookAtSet;

  vtkm::Vec< vtkm::Float32, 3> Look;
  vtkm::Vec< vtkm::Float32, 3> Up;
  vtkm::Vec< vtkm::Float32, 3> LookAt;
  vtkm::Vec< vtkm::Float32, 3> Position;
public:
  VTKM_CONT_EXPORT
  Camera()
  {
  	Height = 500;
  	Width = 500;
  	FovY = 30.f;
  	FovX = 30.f;
  	Zoom = 1.f;
  	Look[0] = 0.f;
  	Look[1] = 0.f;
  	Look[2] = -1.f;
  	LookAt[0] = 0.f;
  	LookAt[1] = 0.f;
  	LookAt[2] = -1.f;
  	Up[0] = 0.f;
  	Up[1] = 1.f;
  	Up[2] = 0.f;
  	Position[0] = 0.f;
  	Position[1] = 0.f;
  	Position[2] = 0.f;  
  	IsViewDirty = true;
  	IsResDirty = true;
  	MortonSort = false;
  }

  VTKM_CONT_EXPORT
  void SetParameters(View3D &view)
  {
    this->SetUp(view.Up);
    this->SetLookAt(view.LookAt);
    this->SetPosition(view.Position);
    this->SetFieldOfView(view.FieldOfView);
    this->SetHeight(view.Height);
    this->SetWidth(view.Width);
  }


  VTKM_CONT_EXPORT
  void SetHeight(const vtkm::Int32 &height)
  {
  	if(height <= 0)  throw vtkm::cont::ErrorControlBadValue("Camera height must be greater than zero.");
  	if(Height != height)
  	{
  		IsResDirty = true;
  		Height = height;
  		this->SetFieldOfView(FovX);
  	} 
  }

  VTKM_CONT_EXPORT
  vtkm::Int32 GetHeight() const
  {
    return Height;
  }

  VTKM_CONT_EXPORT
  void SetWidth(const vtkm::Int32 &width)
  {
  	if(width <= 0)  throw vtkm::cont::ErrorControlBadValue("Camera width must be greater than zero.");
  	if(Width != width)
  	{
  	  IsResDirty = true;
  	  Width = width;
  	  this->SetFieldOfView(FovX);
  	}
  }
  
  VTKM_CONT_EXPORT
  vtkm::Int32 GetWidth() const
  {
    return Width;
  }
  
  VTKM_CONT_EXPORT
  void SetZoom(const vtkm::Float32 &zoom)
  {
    if(zoom <= 0)  throw vtkm::cont::ErrorControlBadValue("Camera zoom must be greater than zero.");
    if(Zoom != zoom)
    {
      IsViewDirty = true;
      Zoom = zoom;
    } 
  }

  VTKM_CONT_EXPORT
  vtkm::Float32 GetZoom() const
  {
    return Zoom;
  }

  VTKM_CONT_EXPORT
  void SetFieldOfView(const vtkm::Float32 &degrees)
  {
  	if(degrees <= 0)  throw vtkm::cont::ErrorControlBadValue("Camera feild of view must be greater than zero.");
  	if(degrees > 180)  throw vtkm::cont::ErrorControlBadValue("Camera feild of view must be less than 180.");
  	// fov is stored as a half angle
  	vtkm::Float32 newFOVY = (vtkm::Float32(Height) / vtkm::Float32(Width)) * degrees;
  	vtkm::Float32 newFOVX = degrees;
  	if(newFOVX != FovX) IsViewDirty = true;
  	if(newFOVY != FovY) IsViewDirty = true;
  	FovX = newFOVX;
    FovY = newFOVY;
  }

  VTKM_CONT_EXPORT
  vtkm::Float32 GetFieldOfView() const
  {
    return FovX;
  }

  VTKM_CONT_EXPORT
  void SetUp(const vtkm::Vec<vtkm::Float32, 3> &up)
  {
  	if(Up != up)
  	{
      Up = up;
  	  vtkm::Normalize(Up);
      IsViewDirty = true;
  	}
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3> GetUp() const
  {
    return Up;
  }

  VTKM_CONT_EXPORT
  void SetLookAt(const vtkm::Vec<vtkm::Float32, 3> &lookAt)
  {
    if(LookAt != lookAt)
    {
      LookAt = lookAt;
      IsViewDirty = true;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3> GetLookAt() const
  {
    return LookAt;
  }

  VTKM_CONT_EXPORT
  void SetPosition(const vtkm::Vec<vtkm::Float32, 3> &position)
  {
    if(LookAt != position)
    {
      Position = position;
      IsViewDirty = true;
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3> GetPosition() const
  {
    return Position;
  }

  VTKM_CONT_EXPORT
  void ResetIsViewDirty()
  {
    IsViewDirty = false;
  }

  VTKM_CONT_EXPORT
  bool GetIsViewDirty() const
  {
    return IsViewDirty;
  }

  VTKM_CONT_EXPORT
  void CreateRays(Ray &rays)
  {

    if(IsResDirty) rays.resize(Height * Width);
    IsResDirty = false;
    //Set the origin of the ray back to the camera position
    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32>( Position[0] ) )
      .Invoke( rays.OriginX );

    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32>( Position[1] ) )
      .Invoke( rays.OriginY );

    vtkm::worklet::DispatcherMapField< MemSet< vtkm::Float32 > >( MemSet< vtkm::Float32>( Position[2] ) )
      .Invoke( rays.OriginZ );

    //Reset the camera look vector
    Look = LookAt - Position;
    vtkm::Normalize(Look);  
    std::cout<<"******Loook "<<Look<<std::endl;
    std::cout<<"******Pos "<<Position<<std::endl;
    //Create the ray direction
    vtkm::worklet::DispatcherMapField< PerspectiveRayGen >( PerspectiveRayGen(Width, 
                                                                              Height, 
                                                                              FovX, 
                                                                              FovY, 
                                                                              Look, 
                                                                              Up, 
                                                                              Zoom) )
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
  void CreateRays(VolumeRay &rays)
  {

    if(IsResDirty) rays.resize(Height * Width);
    IsResDirty = false;

    //Reset the camera look vector
    Look = LookAt - Position;
    vtkm::Normalize(Look);  
    std::cout<<"******Loook "<<Look<<std::endl;
    std::cout<<"******Pos "<<Position<<std::endl;
    //Create the ray direction
    vtkm::worklet::DispatcherMapField< PerspectiveRayGen >( PerspectiveRayGen(Width, 
                                                                              Height, 
                                                                              FovX, 
                                                                              FovY, 
                                                                              Look, 
                                                                              Up, 
                                                                              Zoom) )
      .Invoke(rays.DirX,
              rays.DirY,
              rays.DirZ); //X Y Z

  } //create rays
}; // class camera
}}}//namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Camera_h