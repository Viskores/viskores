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
  ErrorIO.h
  DecodePNG.h
  EncodePNG.h
)

set(sources
  DecodePNG.cxx
  EncodePNG.cxx
)

vtkm_declare_headers(${headers})

vtkm_library( NAME vtkm_io
              SOURCES ${sources}
              HEADERS ${headers}
            )

target_link_libraries(vtkm_io PRIVATE vtkm_lodepng)

add_subdirectory(internal)
add_subdirectory(reader)
add_subdirectory(writer)
