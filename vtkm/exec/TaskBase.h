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
#ifndef vtk_m_exec_TaskBase_h
#define vtk_m_exec_TaskBase_h

#include <vtkm/Types.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

namespace vtkm
{
namespace exec
{

/// Base class for all classes that are used to marshal data from the invocation
/// parameters to the user worklets when invoked in the execution environment.
class TaskBase
{
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_TaskBase_h
