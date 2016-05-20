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
    RenderSurfaceRayTracer(std::size_t width=1024, std::size_t height=1024,
                  const vtkm::rendering::Color &color=vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
        : RenderSurface(width,height,color)
    {
      this->ColorArray = vtkm::cont::make_ArrayHandle(this->ColorBuffer);
      this->DepthArray = vtkm::cont::make_ArrayHandle(this->DepthBuffer);
    }

    class ClearBuffers : public vtkm::worklet::WorkletMapField
    {
        vtkm::rendering::Color ClearColor;
        vtkm::Id NumPixels;
    public:
      VTKM_CONT_EXPORT
      ClearBuffers(const vtkm::rendering::Color &clearColor,
                   vtkm::Id numPixels)
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
        of<<"P6"<<std::endl<<this->Width<<" "<<this->Height<<std::endl<<255<<std::endl;
        int height = static_cast<int>(this->Height);
        for (int yIndex=height-1; yIndex>=0; yIndex--)
            for (std::size_t xIndex=0; xIndex < this->Width; xIndex++)
            {
                const vtkm::Float32 *tuple =
                    &(this->ColorBuffer[static_cast<std::size_t>(yIndex)*this->Width*4 + xIndex*4]);
                of<<(unsigned char)(tuple[0]*255);
                of<<(unsigned char)(tuple[1]*255);
                of<<(unsigned char)(tuple[2]*255);
            }
        of.close();
    }
    VTKM_CONT_EXPORT
    virtual void Clear()
    {
      this->ColorArray = vtkm::cont::make_ArrayHandle(this->ColorBuffer);
      this->DepthArray = vtkm::cont::make_ArrayHandle(this->DepthBuffer);
      vtkm::worklet::DispatcherMapField< ClearBuffers >(
            ClearBuffers( this->BackgroundColor,
                          static_cast<vtkm::Int32>(this->Width*this->Height) ) )
        .Invoke( this->DepthArray,
                 vtkm::exec::ExecutionWholeArray<vtkm::Float32>(this->ColorArray) );
    }

    vtkm::cont::ArrayHandle<vtkm::Float32> ColorArray;
    vtkm::cont::ArrayHandle<vtkm::Float32> DepthArray;

};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurfaceRayTracer_h
