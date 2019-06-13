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
  Conversions.h
  ConvertToRGB.h
  ConvertToRGBA.h
  LookupTable.h
  Portals.h
  ShiftScaleToRGB.h
  ShiftScaleToRGBA.h
  TransferFunction.h
  )

set_source_files_properties(Conversions.h
                            ConvertToRGB.h
                            ConvertToRGBA.h
                            ShiftScaleToRGB.h
                            ShiftScaleToRGBA.h
                            PROPERTIES VTKm_CANT_BE_HEADER_TESTED TRUE)
#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
