##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(multi_block_headers
  AmrArrays.h
  MergeDataSets.h
  )

set(multi_block_sources
  AmrArrays.cxx
  MergeDataSets.cxx
  )

viskores_library(
  NAME viskores_filter_multi_block
  HEADERS ${multi_block_headers}
  DEVICE_SOURCES ${multi_block_sources}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_multi_block)
