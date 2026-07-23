##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
viskores_declare_headers(${headers})
