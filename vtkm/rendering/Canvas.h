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

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT Canvas
{
public:
  using ColorBufferType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
  using DepthBufferType = vtkm::cont::ArrayHandle<vtkm::Float32>;

  Canvas(vtkm::Id width = 1024, vtkm::Id height = 1024);
  virtual ~Canvas();

  virtual vtkm::rendering::Canvas* NewCopy() const;

  virtual void Initialize();

  virtual void Activate();

  virtual void Clear();

  virtual void Finish();

  virtual void BlendBackground();

  VTKM_CONT
  vtkm::Id GetWidth() const { return this->Width; }

  VTKM_CONT
  vtkm::Id GetHeight() const { return this->Height; }

  VTKM_CONT
  const ColorBufferType& GetColorBuffer() const { return this->ColorBuffer; }

  VTKM_CONT
  ColorBufferType& GetColorBuffer() { return this->ColorBuffer; }

  VTKM_CONT
  const DepthBufferType& GetDepthBuffer() const { return this->DepthBuffer; }

  VTKM_CONT
  DepthBufferType& GetDepthBuffer() { return this->DepthBuffer; }

  VTKM_CONT
  void ResizeBuffers(vtkm::Id width, vtkm::Id height)
  {
    VTKM_ASSERT(width >= 0);
    VTKM_ASSERT(height >= 0);

    vtkm::Id numPixels = width * height;
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
  const vtkm::rendering::Color& GetBackgroundColor() const { return this->BackgroundColor; }

  VTKM_CONT
  void SetBackgroundColor(const vtkm::rendering::Color& color) { this->BackgroundColor = color; }

  // If a subclass uses a system that renderers to different buffers, then
  // these should be overridden to copy the data to the buffers.
  virtual void RefreshColorBuffer() const {}
  virtual void RefreshDepthBuffer() const {}

  virtual void SetViewToWorldSpace(const vtkm::rendering::Camera&, bool) {}
  virtual void SetViewToScreenSpace(const vtkm::rendering::Camera&, bool) {}
  virtual void SetViewportClipping(const vtkm::rendering::Camera&, bool) {}

  virtual void SaveAs(const std::string& fileName) const;

  /// Creates a WorldAnnotator of a type that is paired with this Canvas. Other
  /// types of world annotators might work, but this provides a default.
  ///
  /// The WorldAnnotator is created with the C++ new keyword (so it should be
  /// deleted with delete later). A pointer to the created WorldAnnotator is
  /// returned.
  ///
  virtual vtkm::rendering::WorldAnnotator* CreateWorldAnnotator() const;

  VTKM_CONT
  virtual void AddColorSwatch(const vtkm::Vec<vtkm::Float64, 2>& point0,
                              const vtkm::Vec<vtkm::Float64, 2>& point1,
                              const vtkm::Vec<vtkm::Float64, 2>& point2,
                              const vtkm::Vec<vtkm::Float64, 2>& point3,
                              const vtkm::rendering::Color& color) const;

  VTKM_CONT
  void AddColorSwatch(const vtkm::Float64 x0,
                      const vtkm::Float64 y0,
                      const vtkm::Float64 x1,
                      const vtkm::Float64 y1,
                      const vtkm::Float64 x2,
                      const vtkm::Float64 y2,
                      const vtkm::Float64 x3,
                      const vtkm::Float64 y3,
                      const vtkm::rendering::Color& color) const;

  VTKM_CONT
  virtual void AddLine(const vtkm::Vec<vtkm::Float64, 2>& point0,
                       const vtkm::Vec<vtkm::Float64, 2>& point1,
                       vtkm::Float32 linewidth,
                       const vtkm::rendering::Color& color) const;

  VTKM_CONT
  void AddLine(vtkm::Float64 x0,
               vtkm::Float64 y0,
               vtkm::Float64 x1,
               vtkm::Float64 y1,
               vtkm::Float32 linewidth,
               const vtkm::rendering::Color& color) const;

  VTKM_CONT
  virtual void AddColorBar(const vtkm::Bounds& bounds,
                           const vtkm::rendering::ColorTable& colorTable,
                           bool horizontal) const;

  VTKM_CONT
  void AddColorBar(vtkm::Float32 x,
                   vtkm::Float32 y,
                   vtkm::Float32 width,
                   vtkm::Float32 height,
                   const vtkm::rendering::ColorTable& colorTable,
                   bool horizontal) const;

  virtual void AddText(const vtkm::Vec<vtkm::Float32, 2>& position,
                       vtkm::Float32 scale,
                       vtkm::Float32 angle,
                       vtkm::Float32 windowAspect,
                       const vtkm::Vec<vtkm::Float32, 2>& anchor,
                       const vtkm::rendering::Color& color,
                       const std::string& text) const;

  VTKM_CONT
  void AddText(vtkm::Float32 x,
               vtkm::Float32 y,
               vtkm::Float32 scale,
               vtkm::Float32 angle,
               vtkm::Float32 windowAspect,
               vtkm::Float32 anchorX,
               vtkm::Float32 anchorY,
               const vtkm::rendering::Color& color,
               const std::string& text) const;

  friend class AxisAnnotation2D;
  friend class ColorBarAnnotation;
  friend class ColorLegendAnnotation;
  friend class TextAnnotationScreen;

private:
  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::rendering::Color BackgroundColor;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_Canvas_h
