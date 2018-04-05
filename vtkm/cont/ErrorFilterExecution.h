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
#ifndef vtk_m_cont_ErrorFilterExecution_h
#define vtk_m_cont_ErrorFilterExecution_h

#include <vtkm/cont/Error.h>

namespace vtkm
{
namespace cont
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is primarily intended to filters to throw in the control
/// environment to indicate an execution failure due to misconfiguration e.g.
/// incorrect parameters, etc. This is a device independent exception i.e. when
/// thrown, unlike most other exceptions, VTK-m will not try to re-execute the
/// filter on another available device.
class VTKM_ALWAYS_EXPORT ErrorFilterExecution : public vtkm::cont::Error
{
public:
  ErrorFilterExecution(const std::string& message)
    : Error(message, /*is_device_independent=*/true)
  {
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace vtkm::cont

#endif
