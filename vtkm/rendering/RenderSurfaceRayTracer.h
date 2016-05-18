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
#ifndef vtk_m_rendering_RenderSurfaceRayTracer_h
#define vtk_m_rendering_RenderSurfaceRayTracer_h

#include <vtkm/Types.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/RenderSurface.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class RenderSurfaceRayTracer : public RenderSurface
{
public:
    VTKM_CONT_EXPORT
    RenderSurfaceRayTracer(std::size_t w=1024, std::size_t h=1024,
                  const vtkm::rendering::Color &c=vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
        : RenderSurface(w,h,c)
    {
      ColorBuffer = vtkm::cont::make_ArrayHandle(rgba);
      DepthBuffer = vtkm::cont::make_ArrayHandle(zbuff);
    }

    class ClearBuffers : public vtkm::worklet::WorkletMapField
    {
        vtkm::rendering::Color ClearColor;
        vtkm::Int32 NumPixels;
    public:
      VTKM_CONT_EXPORT
      ClearBuffers(const vtkm::rendering::Color &clearColor,
                   vtkm::Int32 numPixels)
      : ClearColor(clearColor),
        NumPixels(numPixels)
      {}
      typedef void ControlSignature(FieldOut<>,
                                    ExecObject);
      typedef void ExecutionSignature(_1,
                                      _2,
                                      WorkIndex);
      VTKM_EXEC_EXPORT
      void operator()(vtkm::Float32 &depth,
                      vtkm::exec::ExecutionWholeArray<vtkm::Float32> &colorBuffer,
                      const vtkm::Id &index) const
      {
        if(index >= NumPixels) return;
        depth = 1.001f;
        vtkm::Id offset = index * 4;
        colorBuffer.Set(offset + 0, ClearColor.Components[0]);
        colorBuffer.Set(offset + 1, ClearColor.Components[1]);
        colorBuffer.Set(offset + 2, ClearColor.Components[2]);
        colorBuffer.Set(offset + 3, ClearColor.Components[3]);
      }
    }; //class ClearBuffers

    VTKM_CONT_EXPORT
    virtual void SaveAs(const std::string &fileName)
    {
        std::ofstream of(fileName.c_str());
        of<<"P6"<<std::endl<<width<<" "<<height<<std::endl<<255<<std::endl;
        int hi = static_cast<int>(height);
        for (int i=hi-1; i>=0; i--)
            for (std::size_t j=0; j < width; j++)
            { 
                const vtkm::Float32 *tuple = &(rgba[i*width*4 + j*4]);
                of<<(unsigned char)(tuple[0]*255);
                of<<(unsigned char)(tuple[1]*255);
                of<<(unsigned char)(tuple[2]*255);
            }
        of.close();
    }
    VTKM_CONT_EXPORT
    virtual void Clear() 
    {
      ColorBuffer = vtkm::cont::make_ArrayHandle(rgba);
      DepthBuffer = vtkm::cont::make_ArrayHandle(zbuff);
      vtkm::worklet::DispatcherMapField< ClearBuffers >( ClearBuffers( bgColor, width*height ) )
        .Invoke( DepthBuffer,
                 vtkm::exec::ExecutionWholeArray<vtkm::Float32>(ColorBuffer) );
    }

    vtkm::cont::ArrayHandle<vtkm::Float32> ColorBuffer;
    vtkm::cont::ArrayHandle<vtkm::Float32> DepthBuffer;

};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurfaceRayTracer_h
