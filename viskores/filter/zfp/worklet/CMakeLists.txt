##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
viskores_declare_headers(${headers})
