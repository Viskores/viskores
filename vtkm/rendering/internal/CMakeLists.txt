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
  OpenGLHeaders.h
  RunTriangulator.h
  )

set_source_files_properties(OpenGLHeaders.h
                            PROPERTIES VTKm_CANT_BE_HEADER_TESTED TRUE)

vtkm_declare_headers(${headers})
