//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_Canvas_h
#define vtk_m_rendering_Canvas_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Matrix.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Texture2D.h>

#define VTKM_DEFAULT_CANVAS_DEPTH 1.001f

namespace vtkm
{
namespace rendering
{

class WorldAnnotator;

class VTKM_RENDERING_EXPORT Canvas
{
public:
  using ColorBufferType = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using DepthBufferType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using FontTextureType = vtkm::rendering::Texture2D<1>;

  Canvas(vtkm::Id width = 1024, vtkm::Id height = 1024);
  virtual ~Canvas();

  virtual vtkm::rendering::Canvas* NewCopy() const;

  virtual void Initialize();

  virtual void Activate();

  virtual void Clear();

  virtual void Finish();

  virtual void BlendBackground();

  VTKM_CONT
  vtkm::Id GetWidth() const;

  VTKM_CONT
  vtkm::Id GetHeight() const;

  VTKM_CONT
  const ColorBufferType& GetColorBuffer() const;

  VTKM_CONT
  ColorBufferType& GetColorBuffer();

  VTKM_CONT
  const DepthBufferType& GetDepthBuffer() const;

  VTKM_CONT
  DepthBufferType& GetDepthBuffer();

  VTKM_CONT
  void ResizeBuffers(vtkm::Id width, vtkm::Id height);

  VTKM_CONT
  const vtkm::rendering::Color& GetBackgroundColor() const;

  VTKM_CONT
  void SetBackgroundColor(const vtkm::rendering::Color& color);

  VTKM_CONT
  const vtkm::rendering::Color& GetForegroundColor() const;

  VTKM_CONT
  void SetForegroundColor(const vtkm::rendering::Color& color);

  VTKM_CONT
  vtkm::Id2 GetScreenPoint(vtkm::Float32 x,
                           vtkm::Float32 y,
                           vtkm::Float32 z,
                           const vtkm::Matrix<vtkm::Float32, 4, 4>& transfor) const;

  // If a subclass uses a system that renderers to different buffers, then
  // these should be overridden to copy the data to the buffers.
  virtual void RefreshColorBuffer() const {}
  virtual void RefreshDepthBuffer() const {}

  virtual void SetViewToWorldSpace(const vtkm::rendering::Camera& camera, bool clip);
  virtual void SetViewToScreenSpace(const vtkm::rendering::Camera& camera, bool clip);
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
  virtual void AddColorSwatch(const vtkm::Vec2f_64& point0,
                              const vtkm::Vec2f_64& point1,
                              const vtkm::Vec2f_64& point2,
                              const vtkm::Vec2f_64& point3,
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
  virtual void AddLine(const vtkm::Vec2f_64& point0,
                       const vtkm::Vec2f_64& point1,
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
                           const vtkm::cont::ColorTable& colorTable,
                           bool horizontal) const;

  VTKM_CONT
  void AddColorBar(vtkm::Float32 x,
                   vtkm::Float32 y,
                   vtkm::Float32 width,
                   vtkm::Float32 height,
                   const vtkm::cont::ColorTable& colorTable,
                   bool horizontal) const;

  virtual void AddText(const vtkm::Vec2f_32& position,
                       vtkm::Float32 scale,
                       vtkm::Float32 angle,
                       vtkm::Float32 windowAspect,
                       const vtkm::Vec2f_32& anchor,
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

  VTKM_CONT
  void AddText(const vtkm::Matrix<vtkm::Float32, 4, 4>& transform,
               vtkm::Float32 scale,
               const vtkm::Vec2f_32& anchor,
               const vtkm::rendering::Color& color,
               const std::string& text,
               const vtkm::Float32& depth = 0) const;


  friend class AxisAnnotation2D;
  friend class ColorBarAnnotation;
  friend class ColorLegendAnnotation;
  friend class TextAnnotationScreen;
  friend class TextRenderer;
  friend class WorldAnnotator;

private:
  bool LoadFont() const;

  const vtkm::Matrix<vtkm::Float32, 4, 4>& GetModelView() const;

  const vtkm::Matrix<vtkm::Float32, 4, 4>& GetProjection() const;

  struct CanvasInternals;
  std::shared_ptr<CanvasInternals> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_Canvas_h
