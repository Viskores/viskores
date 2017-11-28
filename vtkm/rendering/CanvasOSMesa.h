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

  virtual void Initialize() override;

  virtual void RefreshColorBuffer() const override;

  virtual void Activate() override;

  virtual void Finish() override;

  vtkm::rendering::Canvas* NewCopy() const override;

private:
  std::shared_ptr<detail::CanvasOSMesaInternals> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasOSMesa_h
