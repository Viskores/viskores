##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#-----------------------------------------------------------------------------
# Build the configure file.
# need to set numerous VTKm cmake properties to the naming convention
# that we expect for our C++ defines.

set(VTKM_NO_ASSERT ${VTKm_NO_ASSERT})
set(VTKM_NO_ASSERT_CUDA ${VTKm_NO_ASSERT_CUDA})
set(VTKM_NO_ASSERT_HIP ${VTKm_NO_ASSERT_HIP})
set(VTKM_USE_DOUBLE_PRECISION ${VTKm_USE_DOUBLE_PRECISION})
set(VTKM_USE_64BIT_IDS ${VTKm_USE_64BIT_IDS})

set(VTKM_ENABLE_CUDA ${VTKm_ENABLE_CUDA})
set(VTKM_ENABLE_KOKKOS ${VTKm_ENABLE_KOKKOS})
set(VTKM_ENABLE_OPENMP ${VTKm_ENABLE_OPENMP})
set(VTKM_ENABLE_TBB ${VTKm_ENABLE_TBB})

set(VTKM_ENABLE_MPI ${VTKm_ENABLE_MPI})

if(VTKM_ENABLE_CUDA)
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" VTKM_CUDA_VERSION_MAJOR ${CMAKE_CUDA_COMPILER_VERSION})
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" VTKM_CUDA_VERSION_MINOR ${CMAKE_CUDA_COMPILER_VERSION})
endif()

if (TARGET vtkm::kokkos_cuda)
  set(VTKM_KOKKOS_CUDA ON)
elseif(TARGET vtkm::kokkos_hip)
  set(VTKM_KOKKOS_HIP ON)
endif()

set(VTKM_ENABLE_LOGGING ${VTKm_ENABLE_LOGGING})
set(VTKM_NO_DEPRECATED_VIRTUAL ${VTKm_NO_DEPRECATED_VIRTUAL})

vtkm_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  @ONLY)
vtkm_install_headers(vtkm/internal
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  )


set(headers
  ArrayPortalBasic.h
  ArrayPortalDummy.h
  ArrayPortalHelpers.h
  ArrayPortalUniformPointCoordinates.h
  ArrayPortalValueReference.h
  Assume.h
  ConfigureFor32.h
  ConfigureFor64.h
  ConnectivityStructuredInternals.h
  DecayHelpers.h
  ExportMacros.h
  FunctionInterface.h
  FunctionInterfaceDetailPost.h
  FunctionInterfaceDetailPre.h
  IndexTag.h
  IndicesExtrude.h
  Instantiations.h
  Invocation.h
  Meta.h
  Unreachable.h
  VariantImpl.h
  VariantImplDetail.h
  VecOperators.h
  Windows.h
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(headers ${headers}
    ArrayPortalVirtual.h
    )
endif()

vtkm_declare_headers(${headers})

vtkm_pyexpander_generated_file(FunctionInterfaceDetailPre.h)
vtkm_pyexpander_generated_file(FunctionInterfaceDetailPost.h)
vtkm_pyexpander_generated_file(VariantImplDetail.h)
vtkm_pyexpander_generated_file(VecOperators.h)

if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
