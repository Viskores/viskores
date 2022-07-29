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
  AverageByKey.h
  BoundaryTypes.h
  AveragePointNeighborhood.h
  CellDeepCopy.h
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  CosmoTools.h
  DispatcherMapField.h
  DispatcherMapTopology.h
  DispatcherCellNeighborhood.h
  DispatcherPointNeighborhood.h
  DispatcherReduceByKey.h
  FieldStatistics.h
  KdTree3D.h            # Deprecated
  KernelSplatter.h
  Keys.h
  LagrangianStructures.h
  MaskIndices.h
  MaskNone.h
  MaskSelect.h
  MIR.h
  NDimsHistMarginalization.h
  Normalize.h
  ParticleAdvection.h
  Probe.h
  ScalarsToColors.h
  ScatterCounting.h
  ScatterIdentity.h
  ScatterPermutation.h
  ScatterUniform.h
  StableSortIndices.h
  DescriptiveStatistics.h
  StreamLineUniformGrid.h
  StreamSurface.h
  TriangleWinding.h
  WaveletCompressor.h
  WorkletMapField.h
  WorkletMapTopology.h
  WorkletNeighborhood.h
  WorkletCellNeighborhood.h
  WorkletPointNeighborhood.h
  WorkletReduceByKey.h
  )


set(header_impls
  ScalarsToColors.hxx
  Keys.hxx
  )


# This is a list of sources that does not have code that runs on devices. It only
# needs a standard system C++ compiler.
set(sources_no_device
  )

# This is a list of sources that has code that runs on devices and might need to
# be compiled with a device-specific compiler (like CUDA).
set(sources_device
  KeysUnsignedTypes.cxx
  KeysSignedTypes.cxx
  MaskSelect.cxx
  ScatterCounting.cxx
  )


#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(colorconversion)
add_subdirectory(contourtree)
add_subdirectory(contourtree_augmented)
add_subdirectory(contourtree_distributed)
add_subdirectory(cosmotools)
add_subdirectory(lcs)
add_subdirectory(mir)
add_subdirectory(splatkernels)
add_subdirectory(spatialstructure)
add_subdirectory(wavelets)
add_subdirectory(particleadvection)

vtkm_library(
  NAME vtkm_worklet
  SOURCES ${sources_no_device}
  TEMPLATE_SOURCES ${header_impls}
  HEADERS ${headers}
  DEVICE_SOURCES ${sources_device}
  USE_VTKM_JOB_POOL
  )
target_link_libraries(vtkm_worklet PUBLIC vtkm_cont)

set_source_files_properties(${sources_device} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

#-----------------------------------------------------------------------------
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
