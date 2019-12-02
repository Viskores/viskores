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
  ArrayExportMacros.h
  ArrayHandleBasicImpl.h
  ArrayHandleBasicImpl.hxx
  ArrayHandleExecutionManager.h
  ArrayManagerExecution.h
  ArrayManagerExecutionShareWithControl.h
  ArrayPortalFromIterators.h
  ArrayTransfer.h
  AtomicInterfaceControl.h
  AtomicInterfaceExecution.h
  ConnectivityExplicitInternals.h
  DeviceAdapterAlgorithmGeneral.h
  DeviceAdapterListHelpers.h
  FunctorsGeneral.h
  IteratorFromArrayPortal.h
  KXSort.h
  OptionParser.h
  ParallelRadixSort.h
  ParallelRadixSortInterface.h
  ReverseConnectivityBuilder.h
  StorageError.h
  TransferInfo.h
  VariantArrayHandleContainer.h
  VirtualObjectTransfer.h
  VirtualObjectTransferShareWithControl.h
  )

vtkm_declare_headers(${headers})

# These source files are actually compiled in the parent directory.
# They are in a separate directory to highlight which objects are
# internal and which are part of the external interface.
#add_custom_target(vtkmContInternal ALL DEPENDS vtkmCont)

add_subdirectory(testing)
