##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  Color.h
  ColorTable.h
  Plot.h
  RenderSurface.h
  RenderSurfaceOSMesa.h
  RenderSurfaceGL.h
  Scene.h
  SceneRenderer.h
  SceneRendererGL.h
  SceneRendererVolume.h
  Triangulator.h
  View.h
  Window.h
  )

#-----------------------------------------------------------------------------
find_package(MESA REQUIRED)
vtkm_declare_headers(${headers})
add_subdirectory(raytracing)
include_directories(${OSMESA_INCLUDE_DIR})



#-----------------------------------------------------------------------------
add_subdirectory(testing)
