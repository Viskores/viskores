//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
