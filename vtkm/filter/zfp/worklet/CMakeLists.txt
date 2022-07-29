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
  ZFP1DCompressor.h
  ZFP1DDecompress.h
  ZFP2DCompressor.h
  ZFP2DDecompress.h
  ZFPCompressor.h
  ZFPDecompress.h
  )

add_subdirectory(zfp)

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
