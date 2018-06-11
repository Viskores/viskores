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
#ifndef vtk_m_cont_ExecutionObjectBase_h
#define vtk_m_cont_ExecutionObjectBase_h
#include <vtkm/Types.h>
namespace vtkm
{
namespace cont
{
/// Base \c ExecutionObjectBase for execution objects to inherit from so that
/// you can use an arbitrary object as a parameter in an execution environment
/// function. Any method you want to use on the execution side must have the
/// VTKM_EXEC modifier.
/// \tparam Device

class ExecutionObjectBase
{
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ExecutionObjectBase_h
