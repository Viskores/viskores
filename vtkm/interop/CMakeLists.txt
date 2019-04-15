##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  BufferState.h
  TransferToOpenGL.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})

if(VTKm_ENABLE_TESTING AND TARGET vtkm_rendering)
  add_subdirectory(testing)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(internal)

#-----------------------------------------------------------------------------
add_subdirectory(cuda)
