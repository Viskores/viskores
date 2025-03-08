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
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(Histogram CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)
if (Viskores_ENABLE_MPI AND viskores::filter_density_estimate)
  # TODO: this example desperately needs to be updated. The histogram filter has
  # improved immensely since this has originally been written, and the code can
  # be simplified a lot, which will make it more illustrative of using Viskores.
  add_executable(Histogram Histogram.cxx HistogramMPI.h HistogramMPI.cxx)
  target_link_libraries(Histogram PRIVATE viskores::filter_density_estimate MPI::MPI_CXX)
  viskores_add_target_information(Histogram
    DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
    DEVICE_SOURCES HistogramMPI.cxx)
endif()
