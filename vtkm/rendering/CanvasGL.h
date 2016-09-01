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
#ifndef vtk_m_rendering_CanvasGL_h
#define vtk_m_rendering_CanvasGL_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/TextureGL.h>

namespace vtkm {
namespace rendering {

class CanvasGL : public Canvas
{
public:
  VTKM_RENDERING_EXPORT
  CanvasGL(vtkm::Id width=1024,
           vtkm::Id height=1024);

  VTKM_RENDERING_EXPORT
  void Initialize() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void Activate() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void Clear() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void Finish() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  vtkm::rendering::Canvas *NewCopy() const VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void SetViewToWorldSpace(const vtkm::rendering::Camera &camera,
                           bool clip) VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void SetViewToScreenSpace(const vtkm::rendering::Camera &camera,
                            bool clip) VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void SetViewportClipping(const vtkm::rendering::Camera &camera,
                           bool clip) VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void RefreshColorBuffer() const VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  virtual void RefreshDepthBuffer() const VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void AddLine(const vtkm::Vec<vtkm::Float64,2> &point0,
               const vtkm::Vec<vtkm::Float64,2> &point1,
               vtkm::Float32 linewidth,
               const vtkm::rendering::Color &color) const VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  void AddColorBar(const vtkm::Bounds &bounds,
                   const vtkm::rendering::ColorTable &colorTable,
                   bool horizontal) const VTKM_OVERRIDE;


  VTKM_RENDERING_EXPORT
  void AddText(const vtkm::Vec<vtkm::Float32,2> &position,
               vtkm::Float32 scale,
               vtkm::Float32 angle,
               vtkm::Float32 windowAspect,
               const vtkm::Vec<vtkm::Float32,2> &anchor,
               const vtkm::rendering::Color & color,
               const std::string &text) const VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  vtkm::rendering::WorldAnnotator *CreateWorldAnnotator() const VTKM_OVERRIDE;

private:
  vtkm::rendering::BitmapFont Font;
  vtkm::rendering::TextureGL FontTexture;

  VTKM_RENDERING_EXPORT
  void RenderText(vtkm::Float32 scale,
                  const vtkm::Vec<vtkm::Float32,2> &anchor,
                  const std::string &text) const;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasGL_h
