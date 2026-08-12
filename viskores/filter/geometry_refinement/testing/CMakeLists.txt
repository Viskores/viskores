##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(unit_tests
  UnitTestConvertToPointCloud.cxx
  UnitTestExtrusionLinear.cxx
  UnitTestExtrusionRotational.cxx
  UnitTestShrinkFilter.cxx
  UnitTestSplitSharpEdgesFilter.cxx
  UnitTestTetrahedralizeFilter.cxx
  UnitTestTriangulateFilter.cxx
  UnitTestTubeFilter.cxx
  UnitTestVertexClusteringFilter.cxx
  )

set(libraries
  viskores_filter_contour
  viskores_filter_field_conversion
  viskores_filter_geometry_refinement
  viskores_filter_vector_analysis
  viskores_source
  )

if (Viskores_ENABLE_RENDERING)
  list(APPEND libraries
    viskores_rendering_testing
    viskores_rendering)

  list(APPEND unit_tests
    RenderTestSplitSharpEdges.cxx
    )
endif()

viskores_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
)
