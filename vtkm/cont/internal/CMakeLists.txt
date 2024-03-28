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
  ArrayHandleExecutionManager.h
  ArrayPortalFromIterators.h
  ArrayRangeComputeUtils.h
  ArrayTransfer.h
  Buffer.h
  CastInvalidValue.h
  CellLocatorBase.h
  ConnectivityExplicitInternals.h
  ConvertNumComponentsToOffsetsTemplate.h
  DeviceAdapterAlgorithmGeneral.h
  DeviceAdapterMemoryManager.h
  DeviceAdapterMemoryManagerShared.h
  DeviceAdapterListHelpers.h
  FieldCollection.h
  FunctorsGeneral.h
  Hints.h
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
  StorageError.h
  )

vtkm_declare_headers(${headers})

# These source files are actually compiled in the parent directory.
# They are in a separate directory to highlight which objects are
# internal and which are part of the external interface.
#add_custom_target(vtkmContInternal ALL DEPENDS vtkmCont)
