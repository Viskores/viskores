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
  ArrayPortalFromThrust.h
  ExecutionPolicy.h
  IteratorFromArrayPortal.h
  TaskStrided.h
  ThrustPatches.h
  WrappedOperators.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})

set_source_files_properties(ThrustPatches.h
                            PROPERTIES VTKm_CANT_BE_HEADER_TESTED TRUE)

#-----------------------------------------------------------------------------
if (TARGET vtkm::cuda)
  add_subdirectory(testing)
endif()
