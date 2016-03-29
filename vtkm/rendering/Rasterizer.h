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
#ifndef vtk_m_rendering_Rasterizer_h
#define vtk_m_rendering_Rasterizer_h
#include <stdio.h>
#include <vtkm/exec/DepthBufferArray.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/View.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/Matrix.h>
namespace vtkm {
namespace rendering {
//Unary predicate for StreamCompact
struct IsVisible
{
  VTKM_EXEC_CONT_EXPORT bool operator()(const vtkm::Id &x) const
  {
    return (x >= 0);
  }
};

template<class T>
class MemSet : public vtkm::worklet::WorkletMapField
{
  T Value;
public:
  VTKM_CONT_EXPORT
  MemSet(T value)
    : Value(value)
  {
    std::cout<<"Memset value "<<value<<"\n";
  }
  typedef void ControlSignature(FieldOut<>);
  typedef void ExecutionSignature(_1);
  VTKM_EXEC_EXPORT
  void operator()(T &outValue) const
  {
    outValue = Value;
  }
}; //class MemSet

// void WriteColorBuffer(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  &rgba, 
//                       vtkm::Int32 width, 
//                       vtkm::Int32 height)
// {                                                                          
//   FILE *pnm;
//   pnm = fopen("Color.pnm", "w");
//   if( !pnm ) std::cout<<"Could not open pnm file\n";
//   fprintf(pnm, "P%c %d %d 255\n",'6', width, height);
//   vtkm::Int32 size = height * width;
//   unsigned char pixel[3];
//   for(int i = 0; i < size; i++)
//   {
//     vtkm::Vec<vtkm::Float32,4> color = rgba.GetPortalControl().Get(i);
//     pixel[0] = (unsigned char) (color[0]*255.f);
//     pixel[1] = (unsigned char) (color[1]*255.f);
//     pixel[2] = (unsigned char) (color[2]*255.f);
//     fwrite(pixel, sizeof(unsigned char), 3, pnm);
//   }
//   fclose(pnm);
// }

typedef union
  {
    vtkm::Float32 floats[2];
    vtkm::UInt32  ints[2];
    vtkm::UInt64 ulong;
  } Unpacker;
class writer {
public:
static void WriteDepthBufferUnPack(vtkm::cont::ArrayHandle<vtkm::UInt64>  &rgba, 
                            vtkm::Int32 width, 
                            vtkm::Int32 height)
{                                                                          
  FILE *pnm;
  pnm = fopen("DepthPacked.pnm", "w");
  if( !pnm ) std::cout<<"Could not open pnm file\n";
  fprintf(pnm, "P%c %d %d 255\n",'6', width, height);
  vtkm::Int32 size = height * width;
  unsigned char pixel[3];

  for(int i = 0; i < size; i++)
  {
    Unpacker packed;
    packed.ulong = rgba.GetPortalControl().Get(i);
    float depth = packed.floats[1] * 0.5f + 0.5f;
    pixel[0] = (unsigned char)(depth * 255.f);
    pixel[1] = (unsigned char)(depth * 255.f);
    pixel[2] = (unsigned char)(depth * 255.f);
    fwrite(pixel, sizeof(unsigned char), 3, pnm);
  }
  fclose(pnm);
}

static void RasterWriteColorBufferUnPack(vtkm::cont::ArrayHandle<vtkm::UInt64>  &rgba, 
                            vtkm::Int32 width, 
                            vtkm::Int32 height)
{                                                                          
  FILE *pnm;
  pnm = fopen("Packed.pnm", "w");
  if( !pnm ) std::cout<<"Could not open pnm file\n";
  fprintf(pnm, "P%c %d %d 255\n",'6', width, height);
  vtkm::Int32 size = height * width;
  unsigned char pixel[3];
  for(int i = 0; i < size; i++)
  {
    Unpacker packed;
    packed.ulong = rgba.GetPortalControl().Get(i);
    //std::cout<<"Color "<<((packed.ints[0] & 0x00FF0000)>>16)<<" "<<((packed.ints[0] & 0x0000FF00)>>8)<<" "<<(packed.ints[0] & 0x000000FF)<<std::endl;
    pixel[0] = (unsigned char)((packed.ints[0] & 0x00FF0000)>>16);
    pixel[1] = (unsigned char)((packed.ints[0] & 0x0000FF00)>>8);
    pixel[2] = (unsigned char)(packed.ints[0] & 0x000000FF);
    fwrite(pixel, sizeof(unsigned char), 3, pnm);
  }
  fclose(pnm);
}
};


template<typename DeviceAdapter>
class Rasterizer
{
private:
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > FrameBufferHandle;
  typedef typename FrameBufferHandle::ExecutionTypes<DeviceAdapter>::Portal FrameBufferPortal;
  
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> >  ColorArrayHandle;
  typedef typename ColorArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst ColorArrayPortal;

  typedef typename vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > Points32Handle;
  typedef typename Points32Handle::ExecutionTypes<DeviceAdapter>::PortalConst Points32ConstPortal;

  typedef typename vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > Points64Handle;
  typedef typename Points64Handle::ExecutionTypes<DeviceAdapter>::PortalConst Points64ConstPortal;

  typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
  typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;

public:
  
  template<typename HandleType, typename PortalType>
  class RasterUniform : public vtkm::worklet::WorkletMapField
  {
  private:
    typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;
    PortalType Points;
    FrameBufferPortal FrameBuffer;
    vtkm::Matrix<vtkm::Float32,4,4> ViewProjectionMat;
    vtkm::Float32 ScreenWidth;
    vtkm::Float32 ScreenHeight;
    ColorArrayPortal ColorMap;
    vtkm::Float32 ColorMapSize;
    vtkm::Float32 InverseDeltaScalar;
    vtkm::Float32 MinScalar;
    vtkm::exec::DepthBuffer<DeviceAdapter> AArray;
    vtkm::Vec<vtkm::Float32,3> CameraPosition;
  public:
    VTKM_CONT_EXPORT
    RasterUniform(HandleType &points,
                  FrameBufferHandle &frameBuffer,
                  vtkm::Matrix<vtkm::Float32,4,4> viewProjectionMat,
                  vtkm::Int32 screenWidth,
                  vtkm::Int32 screenHeight,
                  ColorArrayHandle colorMap,
                  const vtkm::Float64 scalarBounds[2],
                  const vtkm::exec::DepthBuffer<DeviceAdapter> &aArray,
                  const vtkm::Vec<vtkm::Float32,3> &cameraPosition)
      : Points(points.PrepareForInput( DeviceAdapter() )),
        ViewProjectionMat(viewProjectionMat),
        ColorMap( colorMap.PrepareForInput( DeviceAdapter() )),
        AArray(aArray),
        CameraPosition(cameraPosition)
    {
      this->FrameBuffer
        = frameBuffer.PrepareForOutput(frameBuffer.GetNumberOfValues(), DeviceAdapter() );
      vtkm::rendering::View3D::PrintMatrix(ViewProjectionMat);
      ScreenWidth = vtkm::Float32(screenWidth);
      ScreenHeight = vtkm::Float32(screenHeight);
      InverseDeltaScalar = 1.f / (vtkm::Float32(scalarBounds[1]-scalarBounds[0]));
      MinScalar = vtkm::Float32(scalarBounds[0]);
      ColorMapSize = vtkm::Float32(colorMap.GetNumberOfValues()-1);
      std::cout<<"Raster uniform constructor\n";
      TransformPoint(CameraPosition);
    }
    typedef void ControlSignature(FieldIn<IdType>,
                                  ExecObject,
                                  ExecObject);
    typedef void ExecutionSignature(_1,_2,_3);

    VTKM_EXEC_CONT_EXPORT
    void TransformPoint(vtkm::Vec<Float32,3> &point) const
    {
      vtkm::Vec<Float32,4> temp;
      temp[0] = point[0];
      temp[1] = point[1];
      temp[2] = point[2];
      temp[3] = 1.f;
      temp = vtkm::MatrixMultiply(ViewProjectionMat,temp);
      // perform the perspective divide
      for (vtkm::Int32 i = 0; i < 3; ++i)
      {
        point[i] = temp[i] / temp[3];
      }
    }
    VTKM_EXEC_EXPORT
    void ColorPacker(const vtkm::Float32 &r,
                     const vtkm::Float32 &g,
                     const vtkm::Float32 &b, 
                     vtkm::UInt32 &packedValue) const
    {
      packedValue = 0;
      vtkm::UInt32 temp = vtkm::UInt32(r*255.f);
      temp = temp << 16;
      packedValue = temp | packedValue;
      temp = vtkm::UInt32(g*255.f);
      temp = temp << 8;
      packedValue = temp | packedValue;
      temp = vtkm::UInt32(b*255.f);
      packedValue = temp | packedValue;
    }
    
    template<typename IndicesStorageType,
             typename ScalarType, typename ScalarStorageTag>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &triangleId,
                    vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4>, IndicesStorageType > &indices,
                    vtkm::exec::ExecutionWholeArrayConst<ScalarType, ScalarStorageTag> &scalars) const
    {
      vtkm::Vec<vtkm::Id,4> traingleIndices = indices.Get(triangleId);
      vtkm::Vec<Float32,3> a = Points.Get(traingleIndices[1]);
      vtkm::Vec<Float32,3> b = Points.Get(traingleIndices[2]);
      vtkm::Vec<Float32,3> c = Points.Get(traingleIndices[3]);
      //vtkm::Vec<Float32,4> aColor = ColorMap.Get(traingleIndices[1]);
      //vtkm::Vec<Float32,4> bColor = ColorMap.Get(traingleIndices[2]);
      //vtkm::Vec<Float32,4> cColor = ColorMap.Get(traingleIndices[3]);
      vtkm::Float32 aScalar= scalars.Get(traingleIndices[1]);
      vtkm::Float32 bScalar= scalars.Get(traingleIndices[2]);
      vtkm::Float32 cScalar= scalars.Get(traingleIndices[3]);
      //std::cout<<"Triange Before"<<a<<b<<c<<std::endl;
      //std::cout<<"Triange Scalars"<<aScalar<<" "<<bScalar<<" "<<cScalar<<std::endl;
      TransformPoint(a);
      TransformPoint(b);
      TransformPoint(c);
      vtkm::Vec<vtkm::Int32,2> ai,bi,ci;
      ai[0] = vtkm::Int32((a[0] *.5f + .5f) * ScreenWidth);
      bi[0] = vtkm::Int32((b[0] *.5f + .5f) * ScreenWidth);
      ci[0] = vtkm::Int32((c[0] *.5f + .5f) * ScreenWidth);

      ai[1] = vtkm::Int32((a[1] *.5f + .5f) * ScreenHeight);
      bi[1] = vtkm::Int32((b[1] *.5f + .5f) * ScreenHeight);
      ci[1] = vtkm::Int32((c[1] *.5f + .5f) * ScreenHeight);

      vtkm::Int32 triangleArea =((bi[0]-ai[0])*(ci[1]-ai[1]))-((bi[1]-ai[1])*(ci[0]-ai[0]));
      vtkm::Int32 ymin = vtkm::Min(ai[1],vtkm::Min(bi[1],ci[1]));
      vtkm::Int32 xmin = vtkm::Min(ai[0],vtkm::Min(bi[0],ci[0])); 
      vtkm::Int32 ymax = vtkm::Max(ai[1],vtkm::Max(bi[1],ci[1]));
      vtkm::Int32 xmax = vtkm::Max(ai[0],vtkm::Max(bi[0],ci[0]));
      vtkm::Float32 zmax = vtkm::Max(a[2],vtkm::Max(b[2],c[2]));
      vtkm::Float32 zmin = vtkm::Min(a[2],vtkm::Min(b[2],c[2]));
      ymin = vtkm::Max(0,ymin);
      xmin = vtkm::Max(0,xmin);
      ymax = vtkm::Min(vtkm::Int32(ScreenHeight)-1,ymax);
      xmax = vtkm::Min(vtkm::Int32(ScreenWidth)-1,xmax);
      vtkm::Float32 invTriangleArea = 1.f / vtkm::Float32(triangleArea);  //vtkm::Magnitude(vtkm::Cross(bi-ai, ci-ai));
      bScalar = (bScalar - aScalar) * invTriangleArea;
      cScalar = (cScalar - aScalar) * invTriangleArea;
      vtkm::Float32 z0 = a[2];
      vtkm::Float32 z1 = (b[2] - a[2]) * invTriangleArea;
      vtkm::Float32 z2 = (c[2] - a[2]) * invTriangleArea;
      
      vtkm::Vec<vtkm::Float32,3> e1,e2, normal;   
      e1 = b - a;
      e2 = c - a;
      normal = vtkm::Cross(e1,e2);
      if(invTriangleArea > 0) normal = -normal;
      vtkm::Normalize(normal);
      vtkm::Vec<vtkm::Float32,3> cA = CameraPosition - a;
      vtkm::Normalize(cA);
      vtkm::Vec<vtkm::Float32,3> cB = CameraPosition - b;
      vtkm::Normalize(cB);
      vtkm::Vec<vtkm::Float32,3> cC = CameraPosition - c;
      vtkm::Normalize(cC);

      vtkm::Float32 aDot = vtkm::dot(normal, cA);
      vtkm::Float32 bDot = vtkm::dot(normal, cB);
      vtkm::Float32 cDot = vtkm::dot(normal, cC);
      aDot = vtkm::Max(0.f, aDot);
      bDot = vtkm::Max(0.f, bDot);
      cDot = vtkm::Max(0.f, cDot);

      // std::cout<<normal<<CameraPosition<<std::endl;
      // std::cout<<cA<<cB<<cC<<std::endl;
      // std::cout<<"sDot "<<aDot<<" "<<bDot<<" "<<cDot<<std::endl;

      

      bDot = (bDot - aDot) * invTriangleArea;
      cDot = (cDot - aDot) * invTriangleArea;
      
      invTriangleArea = vtkm::Abs(invTriangleArea);
      //std::cout<<"inv area "<<invTriangleArea<<" "<<std::endl;
      //std::cout<<"Triange After"<<ai<<bi<<ci<<std::endl;


      //It would be nice to know if these were ccw or cw
      
      //find the line coefficients 
      vtkm::Vec<vtkm::Int32,2> ab0,ab1,ab2;
      ab0[0] = (bi[0] - ai[0]); //change for pixel to right
      ab0[1] = (ai[1] - bi[1]); //change for pixel up
      ab1[0] = (ci[0] - bi[0]);
      ab1[1] = (bi[1] - ci[1]);
      ab2[0] = (ai[0] - ci[0]);
      ab2[1] = (ci[1] - ai[1]); 
      //int A01 = v0.y - v1.y, B01 = v1.x - v0.x;
      //int A12 = v1.y - v2.y, B12 = v2.x - v1.x;
      //int A20 = v2.y - v0.y, B20 = v0.x - v2.x;

      vtkm::Int32 w0Start = ab1[0]*(ymin-bi[1]) + ab1[1]*(xmin-bi[0]); // orient2d(v1, v2, p);
      vtkm::Int32 w1Start = ab2[0]*(ymin-ci[1]) + ab2[1]*(xmin-ci[0]);//orient2d(v2, v0, p); //(b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x)
      vtkm::Int32 w2Start = ab0[0]*(ymin-ai[1]) + ab0[1]*(xmin-ai[0]);// orient2d(v0, v1, p); //(b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x)
      //std::cout<<"Triangle BBOX ["<<xmin<<","<<ymin<<"]["<<xmax<<","<<ymax<<"]"<<w0Start<<" "<<w1Start<<" "<<w2Start<<"\n";
      // Rasterize
      for (vtkm::Int32 y = ymin; y <= ymax; y++) 
      {
          // Barycentric coordinates at start of row
          vtkm::Int32 w0 = w0Start;
          vtkm::Int32 w1 = w1Start;
          vtkm::Int32 w2 = w2Start;

          for(vtkm::Int32 x = xmin; x <= xmax; x++) 
          {
              // If p is on or inside all edges, render pixel.
              //if ((sign(w0) == sign(w1))&&(sign(w1) == sign(w2)))
              if ((w0 >= 0 && w1 >=0 && w2>=0) || (w0 <= 0 && w1<= 0 && w2 <= 0))
              {
                vtkm::Float32 lerpedScalar = vtkm::Abs(aScalar + 
                                                       bScalar * vtkm::Float32(w1) + 
                                                       cScalar * vtkm::Float32(w2));
                vtkm::Float32 depth =        vtkm::Abs(z0  + 
                                                       z1 * vtkm::Float32(w1) + 
                                                       z2 * vtkm::Float32(w2));
                vtkm::Float32 lerpedDot =    vtkm::Abs(aDot  + 
                                                       bDot * vtkm::Float32(w1) + 
                                                       cDot * vtkm::Float32(w2));
                lerpedScalar = (lerpedScalar - MinScalar) * InverseDeltaScalar;
                vtkm::Int32 colorIdx = vtkm::Int32(lerpedScalar * ColorMapSize);
                
                // if(colorIdx <0 || colorIdx > ColorMapSize) std::cout<<"COOOOOOLLLLOOROROROROROOROR "<<colorIdx<<" "<<lerpedScalar<<" = "<<vtkm::Abs(aScalar + 
                //                                        bScalar * vtkm::Float32(w1) + 
                //                                        cScalar * vtkm::Float32(w2))<<" - "<<MinScalar<<" / "<<InverseDeltaScalar<<" "<<aScalar<<" "<<bScalar<<" "<<cScalar<<" "<<traingleIndices<<" "<<invTriangleArea<<std::endl;
                vtkm::Vec<vtkm::Float32,4> color = lerpedDot * ColorMap.Get(colorIdx);
                //vtkm::Vec<vtkm::Float32,4> color = ColorMap.Get(colorIdx);
                //std::cout<<"x "<<x<<" y "<<y<<"["<<lerpedScalar<<" "<<depth<<" "<<colorIdx<<" "<<color<<std::endl;
                vtkm::UInt32 packedColor;
                ColorPacker(color[0],color[1],color[2], packedColor);
                AArray.DepthCheck(y*vtkm::Int32(ScreenWidth) +x,depth,packedColor );
                //FrameBuffer.Set(y*ScreenWidth +x, ColorMap.Get(colorIdx));
              }        

              // One step to the right
              w0 += ab1[1];
              w1 += ab2[1];
              w2 += ab0[1];
          }

          // One row step
          w0Start += ab1[0];
          w1Start += ab2[0];
          w2Start += ab0[0];
      }


    }
  }; //class Raster Uniform

  template<typename HandleType, typename PortalType>
  class IsVisibleWorklet : public vtkm::worklet::WorkletMapField
  {
  private:


    vtkm::Float32 ScreenWidth;
    vtkm::Float32 ScreenHeight;
    vtkm::Matrix<vtkm::Float32,4,4> ViewProjectionMat;
    PortalType Points;
  public:
    VTKM_CONT_EXPORT
    IsVisibleWorklet(vtkm::Int32 &screenWidth,
                     vtkm::Int32 &screenHeight,
                     vtkm::Matrix<vtkm::Float32,4,4> &viewProjectionMat,
                     HandleType &points)
      : ScreenWidth(vtkm::Float32(screenWidth)),
        ScreenHeight(vtkm::Float32(screenHeight)),
        ViewProjectionMat(viewProjectionMat),
        Points( points.PrepareForInput( DeviceAdapter() ))
    {
      
    }

    VTKM_EXEC_EXPORT
    void TransformPoint(vtkm::Vec<Float32,3> &point) const
    {
      vtkm::Vec<Float32,4> temp;
      temp[0] = point[0];
      temp[1] = point[1];
      temp[2] = point[2];
      temp[3] = 1.f;
      temp = vtkm::MatrixMultiply(ViewProjectionMat,temp);
      // perform the perspective divide
      for (vtkm::Int32 i = 0; i < 3; ++i)
      {
        point[i] = temp[i] / temp[3];
      }
    }

    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    WorkIndex);

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Vec<vtkm::Id,4> &traingleIndices,
                    vtkm::Id &outputIndex,
                    vtkm::Id &inputIndex) const
    {
      vtkm::Vec<Float32,3> a = Points.Get(traingleIndices[1]);
      vtkm::Vec<Float32,3> b = Points.Get(traingleIndices[2]);
      vtkm::Vec<Float32,3> c = Points.Get(traingleIndices[3]);

      TransformPoint(a);
      TransformPoint(b);
      TransformPoint(c);
      vtkm::Float32 xmin = vtkm::Min(a[0],b[0]);
      vtkm::Float32 xmax = vtkm::Max(a[0],b[0]);
      vtkm::Float32 ymin = vtkm::Min(a[1],b[1]);
      vtkm::Float32 ymax = vtkm::Max(a[1],b[1]);
      vtkm::Float32 zmin = vtkm::Min(a[2],b[2]);
      vtkm::Float32 zmax = vtkm::Max(a[2],b[2]);

      xmin = vtkm::Min(xmin,c[0]);
      xmin = vtkm::Max(xmax,c[0]);
      ymin = vtkm::Min(ymin,c[1]);
      ymin = vtkm::Max(ymax,c[1]);
      zmin = vtkm::Min(zmin,c[2]);
      zmin = vtkm::Max(zmax,c[2]);

      vtkm::Vec<vtkm::Int32,2> ai,bi,ci;
      ai[0] = vtkm::Int32((a[0] *.5f + .5f) * ScreenWidth);
      bi[0] = vtkm::Int32((b[0] *.5f + .5f) * ScreenWidth);
      ci[0] = vtkm::Int32((c[0] *.5f + .5f) * ScreenWidth);

      ai[1] = vtkm::Int32((a[1] *.5f + .5f) * ScreenHeight);
      bi[1] = vtkm::Int32((b[1] *.5f + .5f) * ScreenHeight);
      ci[1] = vtkm::Int32((c[1] *.5f + .5f) * ScreenHeight);

      vtkm::Int32 triangleArea =((bi[0]-ai[0])*(ci[1]-ai[1]))-((bi[1]-ai[1])*(ci[0]-ai[0]));
      
      bool visible = true;
      //valid range is -1 to 1
      //if(triangleArea != 0) std::cout<<" A "<<triangleAre
      if(triangleArea == 0) visible = false;
      if(xmin >  1) visible = false;
      if(xmax < -1) visible = false;
      if(ymin >  1) visible = false;
      if(ymax < -1) visible = false;
      if(zmin >  1) visible = false;
      if(zmax < -1) visible = false;
      //std::cout<<"Area "<<triangleArea<<"ssp "<<xmin<<","<<ymin<<","<<zmin<<"  "<<xmax<<","<<ymax<<","<<zmax<<" | "<<traingleIndices<<std::endl;
      //else 
      //  if(visible) std::cout<<"Visable\n";
      outputIndex = visible ? inputIndex : -1;
    }
  }; //class TransformPointsExplicit

	vtkm::rendering::View3D RasterView;
  vtkm::Matrix<vtkm::Float32,4,4> ViewMatrix;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
  
  VTKM_CONT_EXPORT
  Rasterizer(){}	

  VTKM_CONT_EXPORT
  vtkm::rendering::View3D& GetView()
  {
    return RasterView;
  }
  VTKM_CONT_EXPORT
  void CullNonVisible(const vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  &indices,
                      vtkm::cont::DynamicArrayHandleCoordinateSystem &coordsHandle,
                      vtkm::cont::ArrayHandle< vtkm::Id>  &visibleIndexs)
  {
    vtkm::cont::ArrayHandle< vtkm::Id>  indexes;
    indexes.Allocate(indices.GetNumberOfValues()); //TODO make this a member so there is no realloc
    if(coordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
    {
      vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
      coordsHandle.CastToArrayHandle(vertices);
      vtkm::worklet::DispatcherMapField< IsVisibleWorklet<UniformArrayHandle, UniformConstPortal> >
        ( IsVisibleWorklet<UniformArrayHandle, UniformConstPortal>
            (RasterView.Width,
             RasterView.Height,
             ViewMatrix,
             vertices) 
        )
        .Invoke( indices,
                 indexes );
    }
    else if(coordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> >()))
    {

      vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > vertices;
      coordsHandle.CastToArrayHandle(vertices);
       vtkm::worklet::DispatcherMapField< IsVisibleWorklet<Points32Handle, Points32ConstPortal> >
        ( IsVisibleWorklet<Points32Handle, Points32ConstPortal>
            (RasterView.Width,
             RasterView.Height,
             ViewMatrix,
             vertices) 
        )
        .Invoke( indices,
                 indexes );
    }
    else if(coordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> >()))
    {

      vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > vertices;
      coordsHandle.CastToArrayHandle(vertices);
      vtkm::worklet::DispatcherMapField< IsVisibleWorklet<Points64Handle, Points64ConstPortal> >
        ( IsVisibleWorklet<Points64Handle, Points64ConstPortal>
            (RasterView.Width,
             RasterView.Height,
             ViewMatrix,
             vertices) 
        )
        .Invoke( indices,
                 indexes );
    }
   IsVisible predicate;
   vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::StreamCompact(indexes,
                                                                    indexes,
                                                                    visibleIndexs,
                                                                    predicate);
   std::cout<<"Number of visible triangles "<<visibleIndexs.GetNumberOfValues()<<std::endl;
  }

  VTKM_CONT_EXPORT
  void Run(vtkm::cont::DynamicArrayHandleCoordinateSystem &coordsHandle,
           const vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  &indices,
           vtkm::cont::Field &scalarField)
  {
    vtkm::Float64 scalarBounds[2];
    scalarField.GetBounds(scalarBounds, DeviceAdapter() );
    vtkm::cont::ArrayHandle< vtkm::Float32> scalars;
    // std::cout<<"Casting scalars\n";
    // scalarField.GetData().CastToArrayHandle(scalars);
    // std::cout<<"Done Casting scalars\n";
    // printSummary_ArrayHandle(scalars, std::cout); std::cout<<"\n";
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > frameBuffer;
    vtkm::cont::ArrayHandle<vtkm::UInt64> packedBuffer;
    frameBuffer.Allocate(RasterView.Height * RasterView.Width);
    packedBuffer.Allocate(RasterView.Height * RasterView.Width);
    vtkm::UInt64 packedInitialValue = 0x3f800000FFFFFFFF;
    vtkm::worklet::DispatcherMapField< MemSet< vtkm::UInt64> >( MemSet<vtkm::UInt64>( packedInitialValue ) )
      .Invoke( packedBuffer );
    vtkm::exec::DepthBuffer<DeviceAdapter> zBuffer(packedBuffer);
    //create the view projection matrix
    vtkm::rendering::View3D::PrintMatrix(RasterView.CreateViewMatrix());
    vtkm::rendering::View3D::PrintMatrix(RasterView.CreateProjectionMatrix());
    ViewMatrix 
      = vtkm::MatrixMultiply(RasterView.CreateProjectionMatrix(),
                             RasterView.CreateViewMatrix());
    std::cout<<"ViewProj\n";
    vtkm::rendering::View3D::PrintMatrix(ViewMatrix);
    vtkm::cont::ArrayHandle< vtkm::Id>  visibleTriangles;
    CullNonVisible(indices,coordsHandle,visibleTriangles);
    //if(true) {} else
    if(coordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
    {
      std::cout<<"Uniform Raster\n";
      if(scalarField.GetData().IsTypeAndStorage(vtkm::Float64(), VTKM_DEFAULT_STORAGE_TAG()))
      {
        vtkm::cont::ArrayHandle<vtkm::Float64> scalars = scalarField.GetData().CastToArrayHandle(vtkm::Float64(), VTKM_DEFAULT_STORAGE_TAG());
        vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
        coordsHandle.CastToArrayHandle(vertices);
        vtkm::worklet::DispatcherMapField< RasterUniform<UniformArrayHandle, UniformConstPortal> >
          ( RasterUniform<UniformArrayHandle, UniformConstPortal>(vertices,
                          frameBuffer,
                          ViewMatrix,
                          RasterView.Width,
                          RasterView.Height,
                          ColorMap,
                          scalarBounds,
                          zBuffer,
                          RasterView.Position) 
        )
        .Invoke(visibleTriangles,
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> >(indices),
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Float64>(scalars) );
      }
      else if(scalarField.GetData().IsTypeAndStorage(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG()))
      {
        vtkm::cont::ArrayHandle<vtkm::Float32> scalars = scalarField.GetData().CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
        vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
        coordsHandle.CastToArrayHandle(vertices);
        vtkm::worklet::DispatcherMapField< RasterUniform<UniformArrayHandle, UniformConstPortal> >
          ( RasterUniform<UniformArrayHandle, UniformConstPortal>(vertices,
                          frameBuffer,
                          ViewMatrix,
                          RasterView.Width,
                          RasterView.Height,
                          ColorMap,
                          scalarBounds,
                          zBuffer,
                          RasterView.Position) 
        )
        .Invoke(visibleTriangles,
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> >(indices),
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(scalars) );
      }
    }
    else if(coordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> >()))
    {
      vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > vertices;
      coordsHandle.CastToArrayHandle(vertices);
      if(scalarField.GetData().IsTypeAndStorage(vtkm::Float64(), VTKM_DEFAULT_STORAGE_TAG()))
      {
        vtkm::cont::ArrayHandle<vtkm::Float64> scalars = scalarField.GetData().CastToArrayHandle(vtkm::Float64(), VTKM_DEFAULT_STORAGE_TAG());
        vtkm::worklet::DispatcherMapField< MemSet< vtkm::UInt64> >( MemSet<vtkm::UInt64>( packedInitialValue ) )
           .Invoke( packedBuffer );
        vtkm::worklet::DispatcherMapField< RasterUniform<Points32Handle, Points32ConstPortal> >
        ( RasterUniform<Points32Handle, Points32ConstPortal>(vertices,
                        frameBuffer,
                        ViewMatrix,
                        RasterView.Width,
                        RasterView.Height,
                        ColorMap,
                        scalarBounds,
                        zBuffer,
                        RasterView.Position) 
        )
        .Invoke(visibleTriangles,
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> >(indices),
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Float64>(scalars) );
      }
      else if(scalarField.GetData().IsTypeAndStorage(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG()))
      {
        vtkm::cont::ArrayHandle<vtkm::Float32> scalars = scalarField.GetData().CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
        vtkm::worklet::DispatcherMapField< MemSet< vtkm::UInt64> >( MemSet<vtkm::UInt64>( packedInitialValue ) )
           .Invoke( packedBuffer );
        vtkm::worklet::DispatcherMapField< RasterUniform<Points32Handle, Points32ConstPortal> >
        ( RasterUniform<Points32Handle, Points32ConstPortal>(vertices,
                        frameBuffer,
                        ViewMatrix,
                        RasterView.Width,
                        RasterView.Height,
                        ColorMap,
                        scalarBounds,
                        zBuffer,
                        RasterView.Position) 
        )
        .Invoke(visibleTriangles,
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> >(indices),
                vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(scalars) );
      }
         
    }
    else if(coordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> >()))
    {
      if(scalarField.GetData().IsTypeAndStorage(vtkm::Float64(), VTKM_DEFAULT_STORAGE_TAG()))
      {
        vtkm::cont::ArrayHandle<vtkm::Float64> scalars = scalarField.GetData().CastToArrayHandle(vtkm::Float64(), VTKM_DEFAULT_STORAGE_TAG());
        vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > vertices;
        coordsHandle.CastToArrayHandle(vertices);
        vtkm::worklet::DispatcherMapField< RasterUniform<Points64Handle, Points64ConstPortal> >
          ( RasterUniform<Points64Handle, Points64ConstPortal>(vertices,
                          frameBuffer,
                          ViewMatrix,
                          RasterView.Width,
                          RasterView.Height,
                          ColorMap,
                          scalarBounds,
                          zBuffer,
                          RasterView.Position) 
          )
          .Invoke(visibleTriangles,
                  vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> >(indices),
                  vtkm::exec::ExecutionWholeArrayConst<vtkm::Float64>(scalars) );
      }
      else if(scalarField.GetData().IsTypeAndStorage(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG()))
      {
        vtkm::cont::ArrayHandle<vtkm::Float32> scalars = scalarField.GetData().CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
        vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > vertices;
        coordsHandle.CastToArrayHandle(vertices);
        vtkm::worklet::DispatcherMapField< RasterUniform<Points64Handle, Points64ConstPortal> >
          ( RasterUniform<Points64Handle, Points64ConstPortal>(vertices,
                          frameBuffer,
                          ViewMatrix,
                          RasterView.Width,
                          RasterView.Height,
                          ColorMap,
                          scalarBounds,
                          zBuffer,
                          RasterView.Position) 
          )
          .Invoke(visibleTriangles,
                  vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> >(indices),
                  vtkm::exec::ExecutionWholeArrayConst<vtkm::Float32>(scalars) );
      }

    }
    else throw vtkm::cont::ErrorControlBadValue("Point data type not supported by the rasterizer.");
    std::cout<<"Writing files\n";
    writer::WriteDepthBufferUnPack(packedBuffer,RasterView.Width,RasterView.Height);
    writer::RasterWriteColorBufferUnPack(packedBuffer,RasterView.Width,RasterView.Height);
  
  }
  VTKM_CONT_EXPORT 
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colorMap)
  {
    ColorMap = colorMap;
  }

  
};
}} // namespace vtkm::rendering
#endif //vtk_m_rendering_Rasterizer_h
