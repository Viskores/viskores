##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 Sandia Corporation.
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
#specify that all examples should be quiet in the detection of components
set(VTKm_CONFIGURE_QUIET TRUE)

add_subdirectory(clipping)
add_subdirectory(demo)
add_subdirectory(dynamic_dispatcher)
add_subdirectory(hello_world)
add_subdirectory(isosurface)
add_subdirectory(multi_backend)
add_subdirectory(streamline)
add_subdirectory(tetrahedra)
if(VTKm_BUILD_RENDERING)
  add_subdirectory(rendering)
endif(VTKm_BUILD_RENDERING)

