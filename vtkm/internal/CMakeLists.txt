##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

#-----------------------------------------------------------------------------
# Build the configure file.
# need to set numerous VTKm cmake properties to the naming convention
# that we exepect for our C++ defines.

set(VTKM_USE_DOUBLE_PRECISION ${VTKm_USE_DOUBLE_PRECISION})
set(VTKM_USE_64BIT_IDS ${VTKm_USE_64BIT_IDS})

set(VTKM_ENABLE_CUDA ${VTKm_ENABLE_CUDA})
set(VTKM_ENABLE_TBB ${VTKm_ENABLE_TBB})

set(VTKM_ENABLE_OPENGL_INTEROP ${VTKm_ENABLE_OPENGL_INTEROP})

vtkm_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${CMAKE_BINARY_DIR}/include/${kit_dir}/Configure.h
  @ONLY)
vtkm_install_headers(
  vtkm/internal ${CMAKE_BINARY_DIR}/include/${kit_dir}/Configure.h)

unset(VTKM_ENABLE_OPENGL_INTEROP)

unset(VTKM_ENABLE_TBB)
unset(VTKM_ENABLE_CUDA)

unset(VTKM_USE_64BIT_IDS)
unset(VTKM_USE_DOUBLE_PRECISION)


set(headers
  ArrayPortalUniformPointCoordinates.h
  Assume.h
  ConfigureFor32.h
  ConfigureFor64.h
  ConnectivityStructuredInternals.h
  ExportMacros.h
  FunctionInterface.h
  FunctionInterfaceDetailPost.h
  FunctionInterfaceDetailPre.h
  IndexTag.h
  IntegerSequence.h
  Invocation.h
  ListTagDetail.h
  )

vtkm_declare_headers(${headers})

vtkm_pyexpander_generated_file(FunctionInterfaceDetailPre.h)
vtkm_pyexpander_generated_file(FunctionInterfaceDetailPost.h)

#add_subdirectory(testing)

