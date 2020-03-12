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
set(VTKM_USE_DOUBLE_PRECISION ${VTKm_USE_DOUBLE_PRECISION})
set(VTKM_USE_64BIT_IDS ${VTKm_USE_64BIT_IDS})

set(VTKM_ENABLE_CUDA ${VTKm_ENABLE_CUDA})
set(VTKM_ENABLE_TBB ${VTKm_ENABLE_TBB})
set(VTKM_ENABLE_OPENMP ${VTKm_ENABLE_OPENMP})
set(VTKM_ENABLE_MPI ${VTKm_ENABLE_MPI})

if(VTKM_ENABLE_CUDA)
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" VTKM_CUDA_VERSION_MAJOR ${CMAKE_CUDA_COMPILER_VERSION})
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" VTKM_CUDA_VERSION_MINOR ${CMAKE_CUDA_COMPILER_VERSION})
endif()

set(VTKM_ENABLE_LOGGING ${VTKm_ENABLE_LOGGING})

vtkm_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  @ONLY)
vtkm_install_headers(vtkm/internal
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  )

unset(VTKM_ENABLE_MPI)
unset(VTKM_ENABLE_TBB)
unset(VTKM_ENABLE_CUDA)

unset(VTKM_USE_64BIT_IDS)
unset(VTKM_USE_DOUBLE_PRECISION)
unset(VTKM_NO_ASSERT)


set(headers
  ArrayPortalHelpers.h
  ArrayPortalUniformPointCoordinates.h
  ArrayPortalValueReference.h
  ArrayPortalVirtual.h
  Assume.h
  brigand.hpp
  ConfigureFor32.h
  ConfigureFor64.h
  ConnectivityStructuredInternals.h
  ExportMacros.h
  FunctionInterface.h
  FunctionInterfaceDetailPost.h
  FunctionInterfaceDetailPre.h
  IndexTag.h
  IndicesExtrude.h
  Invocation.h
  Unreachable.h
  Variant.h
  VariantDetail.h
  Windows.h
  )

vtkm_declare_headers(${headers})

vtkm_pyexpander_generated_file(FunctionInterfaceDetailPre.h)
vtkm_pyexpander_generated_file(FunctionInterfaceDetailPost.h)
vtkm_pyexpander_generated_file(VariantDetail.h)

add_subdirectory(testing)

