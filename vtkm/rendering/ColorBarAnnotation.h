//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_ColorBarAnnotation_h
#define vtk_m_rendering_ColorBarAnnotation_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/AxisAnnotation2D.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/ColorTable.h>


namespace vtkm {
namespace rendering {

class ColorBarAnnotation
{
protected:
  vtkm::rendering::ColorTable ColorTable;
  vtkm::rendering::AxisAnnotation2D Axis;

public:
  VTKM_RENDERING_EXPORT
  ColorBarAnnotation();

  VTKM_RENDERING_EXPORT
  virtual ~ColorBarAnnotation();

  VTKM_CONT_EXPORT
  void SetColorTable(const vtkm::rendering::ColorTable &colorTable)
  {
    this->ColorTable = colorTable;
  }

  VTKM_RENDERING_EXPORT
  void SetRange(const vtkm::Range &range, vtkm::IdComponent numTicks);

  VTKM_CONT_EXPORT
  void SetRange(vtkm::Float64 l, vtkm::Float64 h, vtkm::IdComponent numTicks)
  {
    this->SetRange(vtkm::Range(l,h), numTicks);
  }

  VTKM_RENDERING_EXPORT
  virtual void Render(const vtkm::rendering::Camera &camera,
                      const vtkm::rendering::WorldAnnotator &worldAnnotator,
                      vtkm::rendering::Canvas &canvas);
};

}} //namespace vtkm::rendering

#endif // vtk_m_rendering_ColorBarAnnotation_h
