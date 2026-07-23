##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  DeviceAdapterAlgorithmSerial.h
  DeviceAdapterMemoryManagerSerial.h
  DeviceAdapterRuntimeDetectorSerial.h
  DeviceAdapterTagSerial.h
  RuntimeDeviceConfigurationSerial.h
  )

viskores_declare_headers(${headers})

target_sources(viskores_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmSerial.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorSerial.cxx
)
