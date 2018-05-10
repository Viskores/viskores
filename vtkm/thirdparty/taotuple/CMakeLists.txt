##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2018 UT-Battelle, LLC.
##  Copyright 2018 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
add_library(vtkm_taotuple INTERFACE)

vtkm_get_kit_name(kit_name kit_dir)

set(headers
  vtkmtaotuple/include/tao/seq/concatenate.hpp
  vtkmtaotuple/include/tao/seq/config.hpp
  vtkmtaotuple/include/tao/seq/exclusive_scan.hpp
  vtkmtaotuple/include/tao/seq/fold.hpp
  vtkmtaotuple/include/tao/seq/head.hpp
  vtkmtaotuple/include/tao/seq/inclusive_scan.hpp
  vtkmtaotuple/include/tao/seq/integer_sequence.hpp
  vtkmtaotuple/include/tao/seq/is_all.hpp
  vtkmtaotuple/include/tao/seq/is_any.hpp
  vtkmtaotuple/include/tao/seq/make_integer_sequence.hpp
  vtkmtaotuple/include/tao/seq/map.hpp
  vtkmtaotuple/include/tao/seq/max.hpp
  vtkmtaotuple/include/tao/seq/min.hpp
  vtkmtaotuple/include/tao/seq/minus.hpp
  vtkmtaotuple/include/tao/seq/partial_sum.hpp
  vtkmtaotuple/include/tao/seq/plus.hpp
  vtkmtaotuple/include/tao/seq/select.hpp
  vtkmtaotuple/include/tao/seq/sum.hpp
  vtkmtaotuple/include/tao/seq/tail.hpp
  vtkmtaotuple/include/tao/seq/type_by_index.hpp
  vtkmtaotuple/include/tao/seq/values.hpp
  vtkmtaotuple/include/tao/seq/zip.hpp
  vtkmtaotuple/include/tao/tuple/tuple.hpp
)

vtkm_declare_headers(EXCLUDE_FROM_TESTING ${headers})

# taotuple needs C++11
target_compile_features(vtkm_taotuple INTERFACE cxx_auto_type)

target_include_directories(vtkm_taotuple INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}>)

install(TARGETS vtkm_taotuple
  EXPORT ${VTKm_EXPORT_NAME})

## Install headers
install(DIRECTORY vtkmtaotuple/include/
  DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
