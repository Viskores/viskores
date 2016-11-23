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
#ifndef vtk_m_rendering_MapperGL_h
#define vtk_m_rendering_MapperGL_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/CanvasGL.h>

#include <vtkm/rendering/internal/OpenGLHeaders.h>

namespace vtkm {
namespace rendering {

class VTKM_RENDERING_EXPORT MapperGL : public Mapper
{
public:
  MapperGL();

  ~MapperGL();

  void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                   const vtkm::cont::CoordinateSystem &coords,
                   const vtkm::cont::Field &scalarField,
                   const vtkm::rendering::ColorTable &colorTable,
                   const vtkm::rendering::Camera &,
                   const vtkm::Range &scalarRange) VTKM_OVERRIDE;

  void StartScene() VTKM_OVERRIDE;
  void EndScene() VTKM_OVERRIDE;
  void SetCanvas(vtkm::rendering::Canvas *canvas) VTKM_OVERRIDE;

  vtkm::rendering::Mapper *NewCopy() const VTKM_OVERRIDE;

  vtkm::rendering::CanvasGL *Canvas;
  GLuint shader_programme;
  GLfloat mvMat[16], pMat[16];
  bool loaded;
  GLuint vao;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperGL_h
