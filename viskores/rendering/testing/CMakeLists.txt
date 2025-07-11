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

viskores_declare_headers(${headers})

set(unit_tests
  RenderTestAlternateCoordinates.cxx
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

viskores_unit_tests(SOURCES ${unit_tests})
