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
  GuideExampleFilterDataSetWithField.cxx
  GuideExampleGenerateMeshConstantShape.cxx
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

vtkm_find_gl(OPTIONAL GL GLUT)
if(TARGET GLUT::GLUT)
  list(APPEND examples
    GuideExampleRenderingInteractive.cxx
    )
  list(APPEND extra_libs OpenGL::GL GLUT::GLUT)
else()
  message("Not building OpenGL tutorial examples because GL/GLUT not found.")
endif()

vtkm_unit_tests(
  SOURCES ${examples}
  DEVICE_SOURCES ${examples_device}
  LIBRARIES ${extra_libs}
  TEST_ARGS "--no-interaction"
  )

# Special example that is an encapsulated program
# VTKm examples expects vtkm libraries to be namespaced with the prefix vtkm::.
# However as the examples are also built as just another part of the VTK-m code
# those prefix are not added to the targets (This happens during the
# installation). To workaround this issue here, we create IMPORTED libs linking
# to the vtkm libraries used by the examples with expected the vtkm:: prefix.
vtkm_module_get_list(module_list)
foreach(tgt IN LISTS module_list)
  if(TARGET ${tgt})
    # The reason of creating this phony IMPORTED libraries instead of making
    # ALIAS libraries is that ALIAS libraries are GLOBAL whereas IMPORTED are
    # local at the directory level where they are created. We do not want these
    # phony targets to be visible outside of the example directory.
    vtkm_target_mangle(tgt_name_mangled ${tgt})
    add_library("vtkm::${tgt_name_mangled}" INTERFACE IMPORTED)
    target_link_libraries("vtkm::${tgt_name_mangled}" INTERFACE ${tgt})
  endif()
endforeach()
add_library(vtkm::filter INTERFACE IMPORTED)
target_link_libraries(vtkm::filter INTERFACE vtkm_filter)

#add the directory that contains the VTK-m config file to the cmake
#path so that our examples can find VTK-m
set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})

include(VTKmQuickStart.cmake)
set_target_properties(VTKmQuickStart
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
  )
add_test(NAME VTKmQuickStart
   COMMAND VTKmQuickStart ${VTKm_SOURCE_DIR}/data/data/unstructured/cow.vtk
   )
