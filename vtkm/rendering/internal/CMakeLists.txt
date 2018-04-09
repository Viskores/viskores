##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  OpenGLHeaders.h
  RunTriangulator.h
  )

set_source_files_properties(OpenGLHeaders.h
                            PROPERTIES VTKm_CANT_BE_HEADER_TESTED TRUE)

vtkm_declare_headers(${headers})
