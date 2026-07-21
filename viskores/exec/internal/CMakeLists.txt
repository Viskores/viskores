##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  ErrorMessageBuffer.h
  FastVec.h
  ReduceByKeyLookup.h
  TaskSingular.h
  TwoLevelUniformGridExecutionObject.h
  WorkletInvokeFunctorDetail.h
  )

viskores_declare_headers(${headers})

viskores_pyexpander_generated_file(WorkletInvokeFunctorDetail.h)
