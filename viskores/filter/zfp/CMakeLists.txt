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

viskores_library(
  NAME viskores_filter_zfp
  HEADERS ${zfp_headers}
  DEVICE_SOURCES ${zfp_sources_device}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_zfp)

add_subdirectory(worklet)
