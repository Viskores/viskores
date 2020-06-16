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
  ErrorIO.h
  DecodePNG.h
  EncodePNG.h
  ImageReaderBase.h
  ImageReaderPNG.h
  ImageReaderPNM.h
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

add_subdirectory(internal)
add_subdirectory(reader)
add_subdirectory(writer)
add_subdirectory(testing)
