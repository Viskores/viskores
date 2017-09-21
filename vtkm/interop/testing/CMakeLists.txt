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
  TestingOpenGLInterop.h
  TestingTransferFancyHandles.h
  )

set(unit_tests)

# So because we are using GLEW to do our
# GL binding we can only expect one
# of these transfer tests to work
#
#
set(needs_rendering FALSE)
if(VTKm_OSMesa_FOUND AND TARGET vtkm_rendering)
  list(APPEND unit_tests
    UnitTestTransferOSMesa.cxx
    UnitTestFancyTransferOSMesa.cxx
  )
  set(needs_rendering TRUE)
elseif(VTKm_EGL_FOUND AND TARGET vtkm_rendering)
  list(APPEND unit_tests
    UnitTestTransferEGL.cxx
    UnitTestFancyTransferEGL.cxx
  )
  set(needs_rendering TRUE)
else()
  vtkm_configure_component_GLUT()
  if(VTKm_GLUT_FOUND)
    list(APPEND unit_tests
       UnitTestTransferGLUT.cxx
       UnitTestFancyTransferGLUT.cxx
    )
  endif()
endif()


vtkm_declare_headers(${headers} TESTABLE FALSE)
vtkm_unit_tests(SOURCES ${unit_tests})
if(needs_rendering)
  target_link_libraries(UnitTests_vtkm_interop_testing PRIVATE vtkm_rendering)
endif()
