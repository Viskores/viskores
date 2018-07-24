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
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
#add the directory that contains the VTK-m config file to the cmake
#path so that our examples can find VTK-m
set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})

add_subdirectory(clipping)
add_subdirectory(contour_tree)
add_subdirectory(cosmotools)
add_subdirectory(demo)
add_subdirectory(dynamic_dispatcher)
add_subdirectory(game_of_life)
add_subdirectory(hello_world)
add_subdirectory(histogram)
add_subdirectory(isosurface)
add_subdirectory(multi_backend)
add_subdirectory(oscillator)
add_subdirectory(particle_advection)
add_subdirectory(temporal_advection)
add_subdirectory(redistribute_points)
add_subdirectory(rendering)
add_subdirectory(streamline)
add_subdirectory(tetrahedra)
add_subdirectory(unified_memory)
