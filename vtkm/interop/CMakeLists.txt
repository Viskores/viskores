##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  BufferState.h
  TransferToOpenGL.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers} TESTABLE FALSE)

if(VTKm_ENABLE_TESTING AND TARGET vtkm_rendering)
  add_subdirectory(testing)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(internal)

if(TARGET vtkm::cuda)
  add_subdirectory(cuda)
endif()
