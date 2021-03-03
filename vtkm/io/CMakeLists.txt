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
  BOVDataSetReader.h
  DecodePNG.h
  EncodePNG.h
  ErrorIO.h
  FileUtils.h
  ImageReaderBase.h
  ImageReaderPNG.h
  ImageReaderPNM.h
  ImageUtils.h
  ImageWriterBase.h
  ImageWriterPNG.h
  ImageWriterPNM.h
  PixelTypes.h
  VTKDataSetReader.h
  VTKDataSetReaderBase.h
  VTKDataSetWriter.h
  VTKPolyDataReader.h
  VTKRectilinearGridReader.h
  VTKStructuredGridReader.h
  VTKStructuredPointsReader.h
  VTKUnstructuredGridReader.h
  )

set(template_sources
  PixelTypes.hxx
  )

set(sources
  FileUtils.cxx
  DecodePNG.cxx
  EncodePNG.cxx
  )

# TODO: None of these codes actually use a device. Rather, they access ArrayHandle, and we
# currently need to ensure that ArrayHandle is correctly compiled for all devices. This is
# kind of silly, so hopefully sometime in the future you will no longer need to compile for
# devices for ArrayHandle, and this requirement will go away.
set(device_sources
  BOVDataSetReader.cxx
  ImageReaderBase.cxx
  ImageReaderPNG.cxx
  ImageReaderPNM.cxx
  ImageUtils.cxx
  ImageWriterBase.cxx
  ImageWriterPNG.cxx
  ImageWriterPNM.cxx
  VTKDataSetReader.cxx
  VTKDataSetReaderBase.cxx
  VTKDataSetWriter.cxx
  VTKPolyDataReader.cxx
  VTKRectilinearGridReader.cxx
  VTKStructuredGridReader.cxx
  VTKStructuredPointsReader.cxx
  VTKUnstructuredGridReader.cxx
  )

if (VTKm_ENABLE_HDF5_IO)
  set(headers
    ${headers}
    ImageReaderHDF5.h
    ImageWriterHDF5.h)
  set(device_sources
    ${device_sources}
    ImageReaderHDF5.cxx
    ImageWriterHDF5.cxx)
endif ()

vtkm_declare_headers(
  ${headers}
  ${template_sources}
)

vtkm_library(
  NAME vtkm_io
  SOURCES ${sources}
  DEVICE_SOURCES ${device_sources}
  HEADERS ${headers}
  TEMPLATE_SOURCES ${template_sources}
)

target_link_libraries(vtkm_io PUBLIC vtkm_cont PRIVATE vtkm_lodepng)
if (VTKm_ENABLE_HDF5_IO)
  target_include_directories(vtkm_io PRIVATE $<BUILD_INTERFACE:${HDF5_INCLUDE_DIR}>)
  target_link_libraries(vtkm_io PRIVATE ${HDF5_HL_LIBRARIES})
endif()

add_subdirectory(internal)
add_subdirectory(reader)
add_subdirectory(writer)
add_subdirectory(testing)
