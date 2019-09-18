//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_FunctorBase_h
#define vtk_m_exec_FunctorBase_h

#include <vtkm/Types.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace exec
{

/// Base class for all user worklets invoked in the execution environment from a
/// call to vtkm::cont::DeviceAdapterAlgorithm::Schedule.
///
/// This class contains a public method named RaiseError that can be called in
/// the execution environment to signal a problem.
///
class VTKM_ALWAYS_EXPORT FunctorBase
{
public:
  VTKM_EXEC_CONT
  FunctorBase()
    : ErrorMessage()
  {
  }

  VTKM_EXEC
  void RaiseError(const char* message) const { this->ErrorMessage.RaiseError(message); }

  /// Set the error message buffer so that running algorithms can report
  /// errors. This is supposed to be set by the dispatcher. This method may be
  /// replaced as the execution semantics change.
  ///
  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->ErrorMessage = buffer;
  }

private:
  vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_FunctorBase_h
