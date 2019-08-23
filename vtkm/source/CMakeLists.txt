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
  Tangle.h
  )

set(sources
  Tangle.cxx
  )

vtkm_library(NAME vtkm_source
  SOURCES ${sources}
  HEADERS ${headers}
  )

target_link_libraries(vtkm_source PUBLIC vtkm_worklet)