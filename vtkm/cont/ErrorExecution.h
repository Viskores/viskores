//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ErrorExecution_h
#define vtk_m_cont_ErrorExecution_h

#include <vtkm/cont/Error.h>

namespace vtkm
{
namespace cont
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown in the control environment whenever an error occurs in
/// the execution environment.
///
class VTKM_ALWAYS_EXPORT ErrorExecution : public vtkm::cont::Error
{
public:
  ErrorExecution(const std::string& message)
    : Error(message, true)
  {
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ErrorExecution_h
