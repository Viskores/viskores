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
  ArrayCopyUnknown.h
  ArrayHandleDeprecated.h
  ArrayHandleExecutionManager.h
  ArrayPortalFromIterators.h
  ArrayTransfer.h
  AtomicInterfaceControl.h
  AtomicInterfaceExecution.h
  Buffer.h
  CastInvalidValue.h
  CellLocatorBase.h
  ConnectivityExplicitInternals.h
  ConvertNumComponentsToOffsetsTemplate.h
  DeviceAdapterAlgorithmGeneral.h
  DeviceAdapterMemoryManager.h
  DeviceAdapterMemoryManagerShared.h
  DeviceAdapterListHelpers.h
  FunctorsGeneral.h
  IteratorFromArrayPortal.h
  KXSort.h
  MapArrayPermutation.h
  OptionParser.h
  OptionParserArguments.h
  ParallelRadixSort.h
  ParallelRadixSortInterface.h
  PointLocatorBase.h
  ReverseConnectivityBuilder.h
  RuntimeDeviceConfiguration.h
  RuntimeDeviceConfigurationOptions.h
  RuntimeDeviceOption.h
  StorageDeprecated.h
  StorageError.h
  Variant.h
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(headers ${headers}
    TransferInfo.h
    VirtualObjectTransfer.h
    VirtualObjectTransferInstantiate.h
    VirtualObjectTransferShareWithControl.h
    )
endif()

vtkm_declare_headers(${headers})

# These source files are actually compiled in the parent directory.
# They are in a separate directory to highlight which objects are
# internal and which are part of the external interface.
#add_custom_target(vtkmContInternal ALL DEPENDS vtkmCont)

if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
