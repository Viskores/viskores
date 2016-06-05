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
#ifndef vtk_m_rendering_Canvas_h
#define vtk_m_rendering_Canvas_h

#include <vtkm/Types.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/ColorTable.h>

#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class Canvas
{
public:
  VTKM_CONT_EXPORT
  Canvas(vtkm::Id width=1024,
         vtkm::Id height=1024,
         const vtkm::rendering::Color &color =
           vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
    : Width(0), Height(0), BackgroundColor(color)
  {
    this->ResizeBuffers(width, height);
  }

  VTKM_CONT_EXPORT
  virtual void Initialize() = 0;
  VTKM_CONT_EXPORT
  virtual void Activate() = 0;
  VTKM_CONT_EXPORT
  virtual void Clear() = 0;
  VTKM_CONT_EXPORT
  virtual void Finish() = 0;

  typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorBufferType;
  typedef vtkm::cont::ArrayHandle<vtkm::Float32> DepthBufferType;

  VTKM_CONT_EXPORT
  vtkm::Id GetWidth() const { return this->Width; }

  VTKM_CONT_EXPORT
  vtkm::Id GetHeight() const { return this->Height; }

  VTKM_CONT_EXPORT
  const ColorBufferType &GetColorBuffer() const { return this->ColorBuffer; }

  VTKM_CONT_EXPORT
  ColorBufferType &GetColorBuffer() { return this->ColorBuffer; }

  VTKM_CONT_EXPORT
  const DepthBufferType &GetDepthBuffer() const { return this->DepthBuffer; }

  VTKM_CONT_EXPORT
  DepthBufferType &GetDepthBuffer() { return this->DepthBuffer; }

  VTKM_CONT_EXPORT
  void ResizeBuffers(vtkm::Id width, vtkm::Id height)
  {
    VTKM_ASSERT(width >= 0);
    VTKM_ASSERT(height >= 0);

    vtkm::Id numPixels = width*height;
    if (this->ColorBuffer.GetNumberOfValues() != numPixels)
    {
      this->ColorBuffer.Allocate(numPixels);
    }
    if (this->DepthBuffer.GetNumberOfValues() != numPixels)
    {
      this->DepthBuffer.Allocate(numPixels);
    }

    this->Width = width;
    this->Height = height;
  }

  VTKM_CONT_EXPORT
  vtkm::rendering::Color GetBackgroundColor() const
  {
    return this->BackgroundColor;
  }

  VTKM_CONT_EXPORT
  void SetBackgroundColor(const vtkm::rendering::Color &color)
  {
    this->BackgroundColor = color;
  }

  // If a subclass uses a system that renderers to different buffers, then
  // these should be overridden to copy the data to the buffers.
  VTKM_CONT_EXPORT
  virtual void RefreshColorBuffer() {  }
  VTKM_CONT_EXPORT
  virtual void RefreshDepthBuffer() {  }

  VTKM_CONT_EXPORT
  virtual void SetViewToWorldSpace(vtkm::rendering::Camera &, bool) {}
  VTKM_CONT_EXPORT
  virtual void SetViewToScreenSpace(vtkm::rendering::Camera &, bool) {}
  VTKM_CONT_EXPORT
  void SetViewportClipping(vtkm::rendering::Camera &, bool) {}

  VTKM_CONT_EXPORT
  virtual void SaveAs(const std::string &fileName)
  {
    this->RefreshColorBuffer();
    std::ofstream of(fileName.c_str());
    of<<"P6"<<std::endl<<this->Width<<" "<<this->Height<<std::endl<<255<<std::endl;
    ColorBufferType::PortalConstControl colorPortal =
        this->ColorBuffer.GetPortalConstControl();
    for (vtkm::Id yIndex=this->Height-1; yIndex>=0; yIndex--)
    {
      for (vtkm::Id xIndex=0; xIndex < this->Width; xIndex++)
      {
        vtkm::Vec<vtkm::Float32,4> tuple =
            colorPortal.Get(yIndex*this->Width + xIndex);
        of<<(unsigned char)(tuple[0]*255);
        of<<(unsigned char)(tuple[1]*255);
        of<<(unsigned char)(tuple[2]*255);
      }
    }
    of.close();
  }

  virtual void AddLine(vtkm::Float64, vtkm::Float64,
                       vtkm::Float64, vtkm::Float64,
                       vtkm::Float32,
                       const vtkm::rendering::Color &) {}
  virtual void AddColorBar(vtkm::Float32, vtkm::Float32,
                           vtkm::Float32, vtkm::Float32,
                           const vtkm::rendering::ColorTable &,
                           bool) {}
  virtual void AddText(vtkm::Float32, vtkm::Float32,
                       vtkm::Float32,
                       vtkm::Float32,
                       vtkm::Float32,
                       vtkm::Float32, vtkm::Float32,
                       Color,
                       std::string) {}

private:
  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::rendering::Color BackgroundColor;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Canvas_h
