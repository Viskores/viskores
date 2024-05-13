##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

find_package(anari 0.8.0 REQUIRED)

set(headers
  ANARIActor.h
  ANARIMapper.h
  ANARIMapperGlyphs.h
  ANARIMapperPoints.h
  ANARIMapperTriangles.h
  ANARIMapperVolume.h
  ANARIScene.h
  VtkmANARITypes.h
  )

set(sources
  ANARIActor.cxx
  ANARIMapper.cxx
  ANARIScene.cxx
  VtkmANARITypes.cxx
  )

set(device_sources
  ANARIMapperGlyphs.cxx
  ANARIMapperPoints.cxx
  ANARIMapperTriangles.cxx
  ANARIMapperVolume.cxx
  ../../rendering/raytracing/SphereExtractor.cxx
  )


vtkm_library(
  NAME vtkm_anari
  SOURCES ${sources}
  HEADERS ${headers}
  DEVICE_SOURCES ${device_sources}
  )

vtkm_add_target_information(vtkm_anari
  DROP_UNUSED_SYMBOLS
  MODIFY_CUDA_FLAGS
  DEVICE_SOURCES ANARIMapperVolume.cxx
  )

target_link_libraries(vtkm_anari PUBLIC anari::anari)
