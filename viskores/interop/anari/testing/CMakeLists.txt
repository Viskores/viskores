##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(anari_interop_tests
  UnitTestANARIExternalDevice.cxx
  UnitTestANARIOwnership.cxx
  UnitTestANARIMapperGlyphs.cxx
  UnitTestANARIMapperGlyphsData.cxx
  UnitTestANARIMapperAppearance.cxx
  UnitTestANARIMapperPoints.cxx
  UnitTestANARIMapperPointsData.cxx
  UnitTestANARIMapperTrianglesData.cxx
  UnitTestANARIMapperTriangles.cxx
  UnitTestANARIMapperVolume.cxx
  UnitTestANARIStructuredVolume.cxx
  UnitTestANARIUnstructuredVolume.cxx
  UnitTestANARIScene.cxx
  )

set(anari_device_tests
  UnitTestANARIDeviceGeometryCylinder.cxx
  UnitTestANARIDeviceGeometryQuad.cxx
  UnitTestANARIDeviceGeometrySphere.cxx
  )

viskores_unit_tests(SOURCES ${anari_interop_tests} ${anari_device_tests})

set(anari_interop_test_names ${anari_interop_tests})
set(anari_device_test_names ${anari_device_tests})
list(TRANSFORM anari_interop_test_names REPLACE "\\.cxx$" "")
list(TRANSFORM anari_device_test_names REPLACE "\\.cxx$" "")

target_include_directories(UnitTests_viskores_interop_anari_testing PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../../../rendering/anari-device
  )

set_property(TEST ${anari_interop_test_names} APPEND PROPERTY LABELS ANARI_INTEROP)
set_property(TEST UnitTestANARIExternalDevice APPEND PROPERTY LABELS ANARI_EXTERNAL)
set_property(TEST ${anari_device_test_names} APPEND PROPERTY LABELS ANARI_DEVICE)

# The bundled device reports its live ANARI object counts when it is destroyed.
# Treat those diagnostics as failures so every focused test acts as leak coverage,
# including builds that are not instrumented by the project's sanitizer jobs.
set_property(
  TEST ${anari_interop_test_names} ${anari_device_test_names}
  APPEND PROPERTY FAIL_REGULAR_EXPRESSION "detected [1-9][0-9]* leaked ANARI")
