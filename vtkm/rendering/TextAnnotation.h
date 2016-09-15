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
#ifndef vtk_m_rendering_TextAnnotation_h
#define vtk_m_rendering_TextAnnotation_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm {
namespace rendering {

class TextAnnotation
{
public:
  enum HorizontalAlignment
    {
      Left,
      HCenter,
      Right
    };
  enum VerticalAlignment
    {
      Bottom,
      VCenter,
      Top
    };

protected:
  std::string                Text;
  Color                      TextColor;
  vtkm::Float32              Scale;
  vtkm::Vec<vtkm::Float32,2> Anchor;

public:
  VTKM_RENDERING_EXPORT
  TextAnnotation(const std::string &text,
                 const vtkm::rendering::Color &color,
                 vtkm::Float32 scalar);

  VTKM_RENDERING_EXPORT
  virtual ~TextAnnotation();

  VTKM_RENDERING_EXPORT
  void SetText(const std::string &text);

  VTKM_RENDERING_EXPORT
  const std::string &GetText() const;

  /// Set the anchor point relative to the box containing the text. The anchor
  /// is scaled in both directions to the range [-1,1] with -1 at the lower
  /// left and 1 at the upper right.
  ///
  VTKM_RENDERING_EXPORT
  void SetRawAnchor(const vtkm::Vec<vtkm::Float32,2> &anchor);

  VTKM_RENDERING_EXPORT
  void SetRawAnchor(vtkm::Float32 h, vtkm::Float32 v);

  VTKM_RENDERING_EXPORT
  void SetAlignment(HorizontalAlignment h, VerticalAlignment v);

  VTKM_RENDERING_EXPORT
  void SetScale(vtkm::Float32 scale);

  VTKM_RENDERING_EXPORT
  virtual void Render(const vtkm::rendering::Camera &camera,
                      const vtkm::rendering::WorldAnnotator &worldAnnotator,
                      vtkm::rendering::Canvas &canvas) const = 0;
};


}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_TextAnnotation_h
