//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ErrorBadValue_h
#define vtk_m_cont_ErrorBadValue_h

#include <vtkm/cont/Error.h>

namespace vtkm
{
namespace cont
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when a VTKm function or method encounters an invalid
/// value that inhibits progress.
///
class VTKM_ALWAYS_EXPORT ErrorBadValue : public Error
{
public:
  ErrorBadValue(const std::string& message)
    : Error(message, true)
  {
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ErrorBadValue_h
