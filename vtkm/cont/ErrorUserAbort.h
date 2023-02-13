//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ErrorUserAbort_h
#define vtk_m_cont_ErrorUserAbort_h

#include <vtkm/cont/Error.h>

namespace vtkm
{
namespace cont
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when vtk-m detects a request for aborting execution
/// in the current thread
///
class VTKM_ALWAYS_EXPORT ErrorUserAbort : public Error
{
public:
  ErrorUserAbort()
    : Error(Message, true)
  {
  }

private:
  static constexpr const char* Message = "User abort detected.";
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END

}
} // namespace vtkm::cont

#endif // vtk_m_cont_ErrorUserAbort_h
