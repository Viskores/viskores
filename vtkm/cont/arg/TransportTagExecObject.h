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
#ifndef vtk_m_cont_arg_TransportTagExecObject_h
#define vtk_m_cont_arg_TransportTagExecObject_h

#include <vtkm/Types.h>

#include <vtkm/cont/arg/Transport.h>

#include <vtkm/exec/ExecutionObjectBase.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for execution objects.
///
/// \c TransportTagExecObject is a tag used with the \c Transport class to
/// transport objects that work directly in the execution environment.
///
struct TransportTagExecObject
{
};

template <typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagExecObject, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that
  // is not an execution object as an argument that is expected to be one. All
  // execution objects are expected to inherit from
  // vtkm::exec::ExecutionObjectBase.
  VTKM_STATIC_ASSERT_MSG(
    (std::is_base_of<vtkm::exec::ExecutionObjectBase, ContObjectType>::value),
    "All execution objects are expected to inherit from vtkm::exec::ExecutionObjectBase");

  using ExecObjectType = ContObjectType;

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(const ContObjectType& object, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    return object;
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagExecObject_h
