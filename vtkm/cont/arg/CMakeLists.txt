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
  ControlSignatureTagBase.h
  Transport.h
  TransportTagArrayIn.h
  TransportTagArrayInOut.h
  TransportTagArrayOut.h
  TransportTagAtomicArray.h
  TransportTagBitField.h
  TransportTagCellSetIn.h
  TransportTagExecObject.h
  TransportTagKeyedValuesIn.h
  TransportTagKeyedValuesInOut.h
  TransportTagKeyedValuesOut.h
  TransportTagKeysIn.h
  TransportTagTopologyFieldIn.h
  TransportTagWholeArrayIn.h
  TransportTagWholeArrayInOut.h
  TransportTagWholeArrayOut.h
  TypeCheck.h
  TypeCheckTagArray.h
  TypeCheckTagAtomicArray.h
  TypeCheckTagBitField.h
  TypeCheckTagCellSet.h
  TypeCheckTagCellSetStructured.h
  TypeCheckTagExecObject.h
  TypeCheckTagKeys.h
  )

vtkm_declare_headers(${headers})


#-----------------------------------------------------------------------------
add_subdirectory(testing)
