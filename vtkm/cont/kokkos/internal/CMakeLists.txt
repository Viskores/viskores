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
  ArrayManagerExecutionKokkos.h
  DeviceAdapterAlgorithmKokkos.h
  DeviceAdapterMemoryManagerKokkos.h
  DeviceAdapterRuntimeDetectorKokkos.h
  DeviceAdapterTagKokkos.h
  Initialize.h
  ViewTypes.h
  VirtualObjectTransferKokkos.h)

vtkm_declare_headers(${headers})

if (TARGET vtkm::kokkos)
  set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterMemoryManagerKokkos.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorKokkos.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Initialize.cxx)
  target_sources(vtkm_cont PRIVATE ${sources})

  if (TARGET vtkm::kokkos_cuda)
    set_source_files_properties(${sources} TARGET_DIRECTORY vtkm_cont PROPERTIES LANGUAGE CUDA)
  endif()
else()
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorKokkos.cxx)
endif()
