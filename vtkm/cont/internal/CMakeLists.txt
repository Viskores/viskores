##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  ArrayExportMacros.h
  ArrayHandleBasicImpl.h
  ArrayHandleBasicImpl.hxx
  ArrayHandleExecutionManager.h
  ArrayManagerExecution.h
  ArrayManagerExecutionShareWithControl.h
  ArrayPortalFromIterators.h
  ArrayPortalShrink.h
  ArrayTransfer.h
  ConnectivityExplicitInternals.h
  DeviceAdapterAlgorithmGeneral.h
  DeviceAdapterAtomicArrayImplementation.h
  DeviceAdapterDefaultSelection.h
  DeviceAdapterError.h
  DeviceAdapterListHelpers.h
  DeviceAdapterTag.h
  DynamicTransform.h
  FunctorsGeneral.h
  IteratorFromArrayPortal.h
  KXSort.h
  ParallelRadixSort.h
  ParallelRadixSortInterface.h
  ReverseConnectivityBuilder.h
  SimplePolymorphicContainer.h
  StorageError.h
  VirtualObjectTransfer.h
  VirtualObjectTransferShareWithControl.h
  )

vtkm_declare_headers(${headers})

# These source files are actually compiled in the parent directory.
# They are in a separate directory to highlight which objects are
# internal and which are part of the external interface.
#add_custom_target(vtkmContInternal ALL DEPENDS vtkmCont)

add_subdirectory(testing)
