//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
