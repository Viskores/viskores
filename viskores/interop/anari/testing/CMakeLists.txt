##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


viskores_unit_tests(
SOURCES
  UnitTestANARILoadDevice.cxx
  UnitTestANARIOwnership.cxx
  UnitTestANARIMapperGlyphs.cxx
  UnitTestANARIMapperPoints.cxx
  UnitTestANARIMapperTriangles.cxx
  UnitTestANARIMapperVolume.cxx
  UnitTestANARIStructuredVolume.cxx
  UnitTestANARIUnstructuredVolume.cxx
  UnitTestANARIGeometryCylinder.cxx
  UnitTestANARIGeometryQuad.cxx
  UnitTestANARIGeometrySphere.cxx
  UnitTestANARISamplerImage2D.cxx
  UnitTestANARIScene.cxx
  )

target_include_directories(UnitTests_viskores_interop_anari_testing PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../../../rendering/anari-device
  )
