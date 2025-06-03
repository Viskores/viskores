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

set(examples
  GuideExampleArrayHandle.cxx
  GuideExampleArrayHandleCast.cxx
  GuideExampleArrayHandleCompositeVector.cxx
  GuideExampleArrayHandleConstant.cxx
  GuideExampleArrayHandleCoordinateSystems.cxx
  GuideExampleArrayHandleCounting.cxx
  GuideExampleArrayHandleExtractComponent.cxx
  GuideExampleArrayHandleGroupVec.cxx
  GuideExampleArrayHandlePermutation.cxx
  GuideExampleArrayHandleRandom.cxx
  GuideExampleArrayHandleRuntimeVec.cxx
  GuideExampleArrayHandleSwizzle.cxx
  GuideExampleArrayHandleView.cxx
  GuideExampleArrayHandleZip.cxx
  GuideExampleCellShapes.cxx
  GuideExampleColorTables.cxx
  GuideExampleCoreDataTypes.cxx
  GuideExampleEnvironmentModifierMacros.cxx
  GuideExampleInitialization.cxx
  GuideExampleIO.cxx
  GuideExampleLists.cxx
  GuideExampleMatrix.cxx
  GuideExampleNewtonsMethod.cxx
  GuideExampleProvidedFilters.cxx
  GuideExampleRendering.cxx
  GuideExampleRuntimeDeviceTracker.cxx
  GuideExampleTimer.cxx
  GuideExampleTraits.cxx
  GuideExampleTuple.cxx
  )
set(examples_device
  GuideExampleArrayHandleDiscard.cxx
  GuideExampleCellEdgesFaces.cxx
  GuideExampleCellLocator.cxx
  GuideExampleCellOperations.cxx
  GuideExampleDataSetCreation.cxx
  GuideExampleErrorHandling.cxx
  GuideExampleFields.cxx
  GuideExampleFilterDataSetWithField.cxx
  GuideExampleGenerateMeshConstantShape.cxx
  GuideExampleMaskSelect.cxx
  GuideExamplePointLocator.cxx
  GuideExampleScatterCounting.cxx
  GuideExampleScatterPermutation.cxx
  GuideExampleScatterUniform.cxx
  GuideExampleSimpleAlgorithm.cxx
  GuideExampleSimpleHistogram.cxx
  GuideExampleSumOfAngles.cxx
  GuideExampleTriangleQuality.cxx
  GuideExampleUnknownArrayHandle.cxx
  GuideExampleUseWorkletMapField.cxx
  GuideExampleUseWorkletPointNeighborhood.cxx
  GuideExampleUseWorkletReduceByKey.cxx
  GuideExampleUseWorkletVisitCellsWithPoints.cxx
  GuideExampleUseWorkletVisitPointsWithCells.cxx
  )
set(extra_libs)

viskores_find_gl(OPTIONAL GL GLUT)
if(TARGET GLUT::GLUT)
  list(APPEND examples
    GuideExampleRenderingInteractive.cxx
    )
  list(APPEND extra_libs OpenGL::GL GLUT::GLUT)
else()
  message("Not building OpenGL tutorial examples because GL/GLUT not found.")
endif()

viskores_unit_tests(
  SOURCES ${examples}
  DEVICE_SOURCES ${examples_device}
  LIBRARIES ${extra_libs}
  TEST_ARGS "--no-interaction"
  )

# Special example that is an encapsulated program
# Viskores examples expects viskores libraries to be namespaced with the prefix viskores::.
# However as the examples are also built as just another part of the Viskores code
# those prefix are not added to the targets (This happens during the
# installation). To workaround this issue here, we create IMPORTED libs linking
# to the viskores libraries used by the examples with expected the viskores:: prefix.
viskores_module_get_list(module_list)
foreach(tgt IN LISTS module_list)
  if(TARGET ${tgt})
    # The reason of creating this phony IMPORTED libraries instead of making
    # ALIAS libraries is that ALIAS libraries are GLOBAL whereas IMPORTED are
    # local at the directory level where they are created. We do not want these
    # phony targets to be visible outside of the example directory.
    viskores_target_mangle(tgt_name_mangled ${tgt})
    add_library("viskores::${tgt_name_mangled}" INTERFACE IMPORTED)
    target_link_libraries("viskores::${tgt_name_mangled}" INTERFACE ${tgt})
  endif()
endforeach()
add_library(viskores::filter INTERFACE IMPORTED)
target_link_libraries(viskores::filter INTERFACE viskores_filter)

#add the directory that contains the Viskores config file to the cmake
#path so that our examples can find Viskores
set(CMAKE_PREFIX_PATH ${Viskores_BINARY_DIR}/${Viskores_INSTALL_CONFIG_DIR})

include(ViskoresQuickStart.cmake)
set_target_properties(ViskoresQuickStart
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${Viskores_EXECUTABLE_OUTPUT_PATH}
  )
add_test(NAME ViskoresQuickStart
   COMMAND ViskoresQuickStart ${Viskores_SOURCE_DIR}/data/data/unstructured/cow.vtk
   )
