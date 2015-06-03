//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_ExecutionObjectBase_h
#define vtk_m_exec_ExecutionObjectBase_h

namespace vtkm {
namespace exec {

/// Base \c ExecutionObjectBase for execution objects to inherit from so that
/// you can use an arbitrary object as a parameter in an execution environment
/// function. Any method you want to use on the execution side must have the
/// VTKM_EXEC_EXPORT modifier.
///
class ExecutionObjectBase
{

};

}
} // namespace vtkm::exec

#endif //vtk_m_exec_ExecutionObjectBase_h
