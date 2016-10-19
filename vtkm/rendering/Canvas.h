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

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Types.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm {
namespace rendering {

class VTKM_RENDERING_EXPORT Canvas
{
public:
  Canvas(vtkm::Id width=1024,
         vtkm::Id height=1024);

  virtual ~Canvas();

  virtual void Initialize() = 0;

  virtual void Activate() = 0;

  virtual void Clear() = 0;

  virtual void Finish() = 0;

  virtual vtkm::rendering::Canvas *NewCopy() const = 0;

  typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorBufferType;
  typedef vtkm::cont::ArrayHandle<vtkm::Float32> DepthBufferType;

  VTKM_CONT
  vtkm::Id GetWidth() const { return this->Width; }

  VTKM_CONT
  vtkm::Id GetHeight() const { return this->Height; }

  VTKM_CONT
  const ColorBufferType &GetColorBuffer() const { return this->ColorBuffer; }

  VTKM_CONT
  ColorBufferType &GetColorBuffer() { return this->ColorBuffer; }

  VTKM_CONT
  const DepthBufferType &GetDepthBuffer() const { return this->DepthBuffer; }

  VTKM_CONT
  DepthBufferType &GetDepthBuffer() { return this->DepthBuffer; }

  VTKM_CONT
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

  VTKM_CONT
  const vtkm::rendering::Color &GetBackgroundColor() const
  {
    return this->BackgroundColor;
  }

  VTKM_CONT
  void SetBackgroundColor(const vtkm::rendering::Color &color)
  {
    this->BackgroundColor = color;
  }

  // If a subclass uses a system that renderers to different buffers, then
  // these should be overridden to copy the data to the buffers.
  virtual void RefreshColorBuffer() const {  }
  virtual void RefreshDepthBuffer() const  {  }

  virtual void SetViewToWorldSpace(const vtkm::rendering::Camera &, bool) {}
  virtual void SetViewToScreenSpace(const vtkm::rendering::Camera &, bool) {}
  virtual void SetViewportClipping(const vtkm::rendering::Camera &, bool) {}

  virtual void SaveAs(const std::string &fileName) const;

  virtual void AddLine(const vtkm::Vec<vtkm::Float64,2> &point0,
                       const vtkm::Vec<vtkm::Float64,2> &point1,
                       vtkm::Float32 linewidth,
                       const vtkm::rendering::Color &color) const = 0;

  VTKM_CONT
  void AddLine(vtkm::Float64 x0, vtkm::Float64 y0,
               vtkm::Float64 x1, vtkm::Float64 y1,
               vtkm::Float32 linewidth,
               const vtkm::rendering::Color &color) const
  {
    this->AddLine(vtkm::make_Vec(x0, y0),
                  vtkm::make_Vec(x1, y1),
                  linewidth,
                  color);
  }

  virtual void AddColorBar(const vtkm::Bounds &bounds,
                           const vtkm::rendering::ColorTable &colorTable,
                           bool horizontal) const = 0;

  VTKM_CONT
  void AddColorBar(vtkm::Float32 x, vtkm::Float32 y,
                   vtkm::Float32 width, vtkm::Float32 height,
                   const vtkm::rendering::ColorTable &colorTable,
                   bool horizontal) const
  {
    this->AddColorBar(vtkm::Bounds(vtkm::Range(x, x+width),
                                   vtkm::Range(y,y+height),
                                   vtkm::Range(0,0)),
                      colorTable,
                      horizontal);
  }

  virtual void AddText(const vtkm::Vec<vtkm::Float32,2> &position,
                       vtkm::Float32 scale,
                       vtkm::Float32 angle,
                       vtkm::Float32 windowAspect,
                       const vtkm::Vec<vtkm::Float32,2> &anchor,
                       const vtkm::rendering::Color & color,
                       const std::string &text) const = 0;

  VTKM_CONT
  void AddText(vtkm::Float32 x, vtkm::Float32 y,
               vtkm::Float32 scale,
               vtkm::Float32 angle,
               vtkm::Float32 windowAspect,
               vtkm::Float32 anchorX, vtkm::Float32 anchorY,
               const vtkm::rendering::Color & color,
               const std::string &text) const
  {
    this->AddText(vtkm::make_Vec(x, y),
                  scale,
                  angle,
                  windowAspect,
                  vtkm::make_Vec(anchorX, anchorY),
                  color,
                  text);
  }

  /// Creates a WorldAnnotator of a type that is paired with this Canvas. Other
  /// types of world annotators might work, but this provides a default.
  ///
  /// The WorldAnnotator is created with the C++ new keyword (so it should be
  /// deleted with delete later). A pointer to the created WorldAnnotator is
  /// returned.
  ///
  virtual vtkm::rendering::WorldAnnotator *CreateWorldAnnotator() const;

private:
  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::rendering::Color BackgroundColor;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Canvas_h
