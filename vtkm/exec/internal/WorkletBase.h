//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_exec_WorkletBase_h
#define vtkm_exec_WorkletBase_h

#include <vtkm/Types.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

namespace vtkm {
namespace exec {
namespace internal {

/// Base class for all worklet classes. Worklet classes are subclasses and a
/// operator() const is added to implement an algorithm in Dax. Different
/// worklets have different calling semantics.
///
class WorkletBase
{
public:
  VTKM_EXEC_CONT_EXPORT WorkletBase() {  }

  VTKM_EXEC_EXPORT void RaiseError(const char *message) const
  {
    this->ErrorMessage.RaiseError(message);
  }

  /// Set the error message buffer so that running algorithms can report
  /// errors. This is supposed to be set by the dispatcher. This method may be
  /// replaced as the execution semantics change.
  ///
  VTKM_CONT_EXPORT void SetErrorMessageBuffer(
      const vtkm::exec::internal::ErrorMessageBuffer &buffer)
  {
    this->ErrorMessage = buffer;
  }

private:
  vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
};


}
}
} // namespace vtkm::exec::internal

#endif //vtkm_exec_WorkletBase_h
