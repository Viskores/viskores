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
#ifndef vtk_m_rendering_CanvasOSMesa_h
#define vtk_m_rendering_CanvasOSMesa_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/CanvasGL.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

namespace detail
{

struct CanvasOSMesaInternals;

} // namespace detail

class VTKM_RENDERING_EXPORT CanvasOSMesa : public CanvasGL
{
public:
  CanvasOSMesa(vtkm::Id width = 1024, vtkm::Id height = 1024);

  ~CanvasOSMesa();

  virtual void Initialize() VTKM_OVERRIDE;

  virtual void RefreshColorBuffer() const VTKM_OVERRIDE;

  virtual void Activate() VTKM_OVERRIDE;

  virtual void Finish() VTKM_OVERRIDE;

  vtkm::rendering::Canvas* NewCopy() const VTKM_OVERRIDE;

private:
  std::shared_ptr<detail::CanvasOSMesaInternals> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasOSMesa_h
