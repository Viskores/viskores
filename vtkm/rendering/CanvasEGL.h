//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

namespace vtkm
{
namespace rendering
{

namespace detail
{

struct CanvasEGLInternals;
}

class VTKM_RENDERING_EXPORT CanvasEGL : public CanvasGL
{
public:
  CanvasEGL(vtkm::Id width = 1024, vtkm::Id height = 1024);

  ~CanvasEGL();

  virtual void Initialize() override;

  virtual void Activate() override;

  vtkm::rendering::Canvas* NewCopy() const override;

private:
  std::shared_ptr<detail::CanvasEGLInternals> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasEGL_h
