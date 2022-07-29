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
  RenderTest.h
  Testing.h
)

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestCanvas.cxx
  UnitTestMapperConnectivity.cxx
  UnitTestMultiMapper.cxx
  #UnitTestMapperCylinders.cxx
  UnitTestMapperPoints.cxx
  UnitTestMapperQuads.cxx
  UnitTestMapperRayTracer.cxx
  UnitTestMapperWireframer.cxx
  UnitTestMapperVolume.cxx
  UnitTestScalarRenderer.cxx
  UnitTestMapperGlyphScalar.cxx
  UnitTestMapperGlyphVector.cxx
)

set(library_sources
  RenderTest.cxx
  Testing.cxx
  )

vtkm_library(
  NAME vtkm_rendering_testing
  # When rendering code no longer needs a device compiler, this should change to SOURCES
  DEVICE_SOURCES ${library_sources}
  HEADERS ${headers}
  )
target_link_libraries(vtkm_rendering_testing PUBLIC vtkm_cont_testing vtkm_rendering)
target_link_libraries(vtkm_rendering_testing PRIVATE vtkm_io)

vtkm_unit_tests(SOURCES ${unit_tests} ALL_BACKENDS LIBRARIES vtkm_rendering vtkm_rendering_testing)
