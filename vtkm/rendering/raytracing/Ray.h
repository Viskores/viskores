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
#ifndef vtk_m_rendering_raytracing_Ray_h
#define vtk_m_rendering_raytracing_Ray_h
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/DeviceAdapter.h>
namespace vtkm {
namespace rendering {
namespace raytracing {

class Ray
{
public:

  // composite vectors to hold array handles
  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32> >::type Intersection;

  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32> >::type Normal;
  
  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32> >::type Origin;
  
  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32> >::type Dir;

  vtkm::cont::ArrayHandle<vtkm::Float32> IntersectionX; //ray Normal
  vtkm::cont::ArrayHandle<vtkm::Float32> IntersectionY;
  vtkm::cont::ArrayHandle<vtkm::Float32> IntersectionZ;

  vtkm::cont::ArrayHandle<vtkm::Float32> NormalX; //ray Normal
  vtkm::cont::ArrayHandle<vtkm::Float32> NormalY;
  vtkm::cont::ArrayHandle<vtkm::Float32> NormalZ;

  vtkm::cont::ArrayHandle<vtkm::Float32> OriginX; //ray Origin
  vtkm::cont::ArrayHandle<vtkm::Float32> OriginY;
  vtkm::cont::ArrayHandle<vtkm::Float32> OriginZ;

  vtkm::cont::ArrayHandle<vtkm::Float32> DirX; //ray Dir
  vtkm::cont::ArrayHandle<vtkm::Float32> DirY;
  vtkm::cont::ArrayHandle<vtkm::Float32> DirZ;

  vtkm::cont::ArrayHandle<vtkm::Float32> U; //barycentric coordinates
  vtkm::cont::ArrayHandle<vtkm::Float32> V;

  vtkm::cont::ArrayHandle<vtkm::Float32> Distance; //distance to hit

  vtkm::cont::ArrayHandle<vtkm::Float32> Scalar; //scalar

  vtkm::cont::ArrayHandle<vtkm::Id> HitIdx;

  vtkm::Int32 NumRays;
  VTKM_CONT_EXPORT
  Ray()
  { 
    NumRays = 0;
    vtkm::IdComponent inComp[3];
    inComp[0] = 0; 
    inComp[1] = 1; 
    inComp[2] = 2;
    Intersection = vtkm::cont::make_ArrayHandleCompositeVector( IntersectionX, inComp[0],
                                                                IntersectionY, inComp[1],
                                                                IntersectionZ, inComp[2]); 

    Normal = vtkm::cont::make_ArrayHandleCompositeVector( NormalX, inComp[0],
                                                          NormalY, inComp[1],
                                                          NormalZ, inComp[2]);

    Origin = vtkm::cont::make_ArrayHandleCompositeVector( OriginX, inComp[0],
                                                          OriginY, inComp[1],
                                                          OriginZ, inComp[2]);

    Dir  = vtkm::cont::make_ArrayHandleCompositeVector( DirX, inComp[0],
                                                        DirY, inComp[1],
                                                        DirZ, inComp[2]);
  }
  VTKM_CONT_EXPORT
  Ray( const vtkm::Int32 size )
  {
    NumRays = size;

    IntersectionX.Allocate( NumRays );
    IntersectionY.Allocate( NumRays );
    IntersectionZ.Allocate( NumRays );

    NormalX.Allocate( NumRays );
    NormalY.Allocate( NumRays );
    NormalZ.Allocate( NumRays );

    OriginX.Allocate( NumRays );
    OriginY.Allocate( NumRays );
    OriginZ.Allocate( NumRays );

    DirX.Allocate( NumRays );
    DirY.Allocate( NumRays );
    DirZ.Allocate( NumRays );

    U.Allocate( NumRays );
    V.Allocate( NumRays );
    Distance.Allocate( NumRays );
    Scalar.Allocate( NumRays );

    HitIdx.Allocate( NumRays );

    vtkm::IdComponent inComp[3];
    inComp[0] = 0; 
    inComp[1] = 1; 
    inComp[2] = 2;

    Intersection = vtkm::cont::make_ArrayHandleCompositeVector( IntersectionX, inComp[0],
                                                                IntersectionY, inComp[1],
                                                                IntersectionZ, inComp[2]); 
                                                                
    Normal = vtkm::cont::make_ArrayHandleCompositeVector( NormalX, inComp[0],
                                                          NormalY, inComp[1],
                                                          NormalZ, inComp[2]);
                                                          
    Origin = vtkm::cont::make_ArrayHandleCompositeVector( OriginX, inComp[0],
                                                          OriginY, inComp[1],
                                                          OriginZ, inComp[2]);
                                                          
    Dir  = vtkm::cont::make_ArrayHandleCompositeVector( DirX, inComp[0],
                                                        DirY, inComp[1],
                                                        DirZ, inComp[2]);
  }
  VTKM_CONT_EXPORT
  void resize( const vtkm::Int32 newSize )
  {
    if(newSize == NumRays) return; //nothing to do

    NumRays = newSize;

    IntersectionX.Allocate( NumRays );
    IntersectionY.Allocate( NumRays );
    IntersectionZ.Allocate( NumRays );

    NormalX.Allocate( NumRays );
    NormalY.Allocate( NumRays );
    NormalZ.Allocate( NumRays );

    OriginX.Allocate( NumRays );
    OriginY.Allocate( NumRays );
    OriginZ.Allocate( NumRays );

    DirX.Allocate( NumRays );
    DirY.Allocate( NumRays );
    DirZ.Allocate( NumRays );

    U.Allocate( NumRays );
    V.Allocate( NumRays );
    Distance.Allocate( NumRays );
    Scalar.Allocate( NumRays );

    HitIdx.Allocate( NumRays );
  }

};// class ray

class VolumeRay
{
public:
  
  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32> >::type Dir;

  vtkm::cont::ArrayHandle<vtkm::Float32> DirX; //ray Dir
  vtkm::cont::ArrayHandle<vtkm::Float32> DirY;
  vtkm::cont::ArrayHandle<vtkm::Float32> DirZ;

  vtkm::cont::ArrayHandle<vtkm::Float32> MinDistance; //distance to hit
  vtkm::cont::ArrayHandle<vtkm::Float32> MaxDistance; //distance to hit
  vtkm::cont::ArrayHandle<vtkm::Id> HitIndex;
  vtkm::Int32 NumRays;
  VTKM_CONT_EXPORT
  VolumeRay()
  { 
    NumRays = 0;
    vtkm::IdComponent inComp[3];
    inComp[0] = 0; 
    inComp[1] = 1; 
    inComp[2] = 2;

    Dir  = vtkm::cont::make_ArrayHandleCompositeVector( DirX, inComp[0],
                                                        DirY, inComp[1],
                                                        DirZ, inComp[2]);
  }
  VTKM_CONT_EXPORT
  VolumeRay( const vtkm::Int32 size )
  {
    NumRays = size;

    DirX.Allocate( NumRays );
    DirY.Allocate( NumRays );
    DirZ.Allocate( NumRays );

    MinDistance.Allocate( NumRays );
    MaxDistance.Allocate( NumRays );
    HitIndex.Allocate( NumRays );

    vtkm::IdComponent inComp[3];
    inComp[0] = 0; 
    inComp[1] = 1; 
    inComp[2] = 2;

                                                          
    Dir  = vtkm::cont::make_ArrayHandleCompositeVector( DirX, inComp[0],
                                                        DirY, inComp[1],
                                                        DirZ, inComp[2]);
  }
  VTKM_CONT_EXPORT
  void resize( const vtkm::Int32 newSize )
  {
    if(newSize == NumRays) return; //nothing to do

    NumRays = newSize;

    DirX.Allocate( NumRays );
    DirY.Allocate( NumRays );
    DirZ.Allocate( NumRays );

    MinDistance.Allocate( NumRays );
    MaxDistance.Allocate( NumRays );
    HitIndex.Allocate( NumRays );

  }

};// class ray

}}}//namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Ray_h