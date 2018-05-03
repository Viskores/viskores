//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_CanvasGL_h
#define vtk_m_rendering_CanvasGL_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/TextureGL.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT CanvasGL : public Canvas
{
public:
  CanvasGL(vtkm::Id width = 1024, vtkm::Id height = 1024);

  ~CanvasGL();

  void Initialize() override;

  void Activate() override;

  void Clear() override;

  void Finish() override;

  vtkm::rendering::Canvas* NewCopy() const override;

  void SetViewToWorldSpace(const vtkm::rendering::Camera& camera, bool clip) override;

  void SetViewToScreenSpace(const vtkm::rendering::Camera& camera, bool clip) override;

  void SetViewportClipping(const vtkm::rendering::Camera& camera, bool clip) override;

  void RefreshColorBuffer() const override;

  virtual void RefreshDepthBuffer() const override;

  vtkm::rendering::WorldAnnotator* CreateWorldAnnotator() const override;

protected:
  void AddLine(const vtkm::Vec<vtkm::Float64, 2>& point0,
               const vtkm::Vec<vtkm::Float64, 2>& point1,
               vtkm::Float32 linewidth,
               const vtkm::rendering::Color& color) const override;

  void AddColorBar(const vtkm::Bounds& bounds,
                   const vtkm::cont::ColorTable& colorTable,
                   bool horizontal) const override;

  void AddColorSwatch(const vtkm::Vec<vtkm::Float64, 2>& point0,
                      const vtkm::Vec<vtkm::Float64, 2>& point1,
                      const vtkm::Vec<vtkm::Float64, 2>& point2,
                      const vtkm::Vec<vtkm::Float64, 2>& point3,
                      const vtkm::rendering::Color& color) const override;

  void AddText(const vtkm::Vec<vtkm::Float32, 2>& position,
               vtkm::Float32 scale,
               vtkm::Float32 angle,
               vtkm::Float32 windowAspect,
               const vtkm::Vec<vtkm::Float32, 2>& anchor,
               const vtkm::rendering::Color& color,
               const std::string& text) const override;

private:
  vtkm::rendering::BitmapFont Font;
  vtkm::rendering::TextureGL FontTexture;

  void RenderText(vtkm::Float32 scale,
                  const vtkm::Vec<vtkm::Float32, 2>& anchor,
                  const std::string& text) const;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasGL_h
