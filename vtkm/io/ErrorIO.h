//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ErrorIO_h
#define vtk_m_io_ErrorIO_h

#include <vtkm/cont/Error.h>

namespace vtkm
{
namespace io
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKM_ALWAYS_EXPORT ErrorIO : public vtkm::cont::Error
{
public:
  ErrorIO() {}
  ErrorIO(const std::string message)
    : Error(message)
  {
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace vtkm::io

#endif //vtk_m_io_ErrorIO_h
