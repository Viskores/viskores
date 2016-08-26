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

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/shared_ptr.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace rendering {

namespace detail {

struct CanvasOSMesaInternals;

} // namespace detail

class CanvasOSMesa : public CanvasGL
{
public:
  VTKM_RENDERING_EXPORT
  CanvasOSMesa(vtkm::Id width=1024,
               vtkm::Id height=1024);

  VTKM_RENDERING_EXPORT
  ~CanvasOSMesa();

  VTKM_RENDERING_EXPORT
  virtual void Initialize() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  virtual void RefreshColorBuffer() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  virtual void Activate() VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  virtual void Finish() VTKM_OVERRIDE;

private:
  boost::shared_ptr<detail::CanvasOSMesaInternals> Internals;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasOSMesa_h
