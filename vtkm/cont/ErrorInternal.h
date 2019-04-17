//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ErrorInternal_h
#define vtk_m_cont_ErrorInternal_h

#include <vtkm/cont/Error.h>

namespace vtkm
{
namespace cont
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when VTKm detects an internal state that should never
/// be reached. This error usually indicates a bug in vtkm or, at best, VTKm
/// failed to detect an invalid input it should have.
///
class VTKM_ALWAYS_EXPORT ErrorInternal : public Error
{
public:
  ErrorInternal(const std::string& message)
    : Error(message)
  {
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ErrorInternal_h
