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
  ErrorMessageBuffer.h
  FastVec.h
  ReduceByKeyLookup.h
  TaskSingular.h
  TwoLevelUniformGridExecutionObject.h
  WorkletInvokeFunctorDetail.h
  )

viskores_declare_headers(${headers})

viskores_pyexpander_generated_file(WorkletInvokeFunctorDetail.h)
