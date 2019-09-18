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
  ZFPBlockReader.h
  ZFPBlockWriter.h
  ZFPCodec.h
  ZFPDecode.h
  ZFPDecode1.h
  ZFPDecode2.h
  ZFPDecode3.h
  ZFPEncode.h
  ZFPEncode1.h
  ZFPEncode2.h
  ZFPEncode3.h
  ZFPFunctions.h
  ZFPStructs.h
  ZFPTools.h
  ZFPTypeInfo.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
