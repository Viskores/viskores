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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(DynamicDispatcherExample main.cxx)
target_include_directories(DynamicDispatcherExample PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(DynamicDispatcherExample PRIVATE ${VTKm_LIBRARIES})
target_compile_options(DynamicDispatcherExample PRIVATE ${VTKm_COMPILE_OPTIONS})
