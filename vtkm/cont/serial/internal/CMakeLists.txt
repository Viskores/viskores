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
  ArrayManagerExecutionSerial.h
  AtomicInterfaceExecutionSerial.h
  DeviceAdapterAlgorithmSerial.h
  DeviceAdapterRuntimeDetectorSerial.h
  DeviceAdapterTagSerial.h
  ExecutionArrayInterfaceBasicSerial.h
  VirtualObjectTransferSerial.h
  )
vtkm_declare_headers(${headers})

target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/ArrayManagerExecutionSerial.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmSerial.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorSerial.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ExecutionArrayInterfaceBasicSerial.cxx
)
