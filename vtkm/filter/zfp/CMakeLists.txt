##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(zfp_headers
  ZFPCompressor1D.h
  ZFPCompressor2D.h
  ZFPCompressor3D.h
  ZFPDecompressor1D.h
  ZFPDecompressor2D.h
  ZFPDecompressor3D.h
  )

set(zfp_sources_device
  ZFPCompressor1D.cxx
  ZFPCompressor2D.cxx
  ZFPCompressor3D.cxx
  ZFPDecompressor1D.cxx
  ZFPDecompressor2D.cxx
  ZFPDecompressor3D.cxx
  )

vtkm_library(
  NAME vtkm_filter_zfp
  HEADERS ${zfp_headers}
  DEVICE_SOURCES ${zfp_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_zfp PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_zfp)

add_subdirectory(worklet)
#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
