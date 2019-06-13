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
  WorkletInvokeFunctorDetail.h
  TwoLevelUniformGridExecutionObject.h
  )

vtkm_declare_headers(${headers})

vtkm_pyexpander_generated_file(WorkletInvokeFunctorDetail.h)

add_subdirectory(testing)
