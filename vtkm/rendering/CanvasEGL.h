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
#ifndef vtk_m_rendering_CanvasEGL_h
#define vtk_m_rendering_CanvasEGL_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/CanvasGL.h>

#include <memory>

namespace vtkm {
namespace rendering {

namespace detail {

struct CanvasEGLInternals;

}

class CanvasEGL : public CanvasGL
{
public:
  VTKM_RENDERING_EXPORT
  CanvasEGL(vtkm::Id width=1024,
            vtkm::Id height=1024);

  VTKM_RENDERING_EXPORT
  ~CanvasEGL();

  VTKM_RENDERING_EXPORT
  virtual void Initialize() VTKM_OVERRIDE;
    
  VTKM_RENDERING_EXPORT
  virtual void Activate() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  vtkm::rendering::Canvas *NewCopy() const VTKM_OVERRIDE;

private:
  std::shared_ptr<detail::CanvasEGLInternals> Internals;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasEGL_h
