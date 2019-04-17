//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagExecObject_h
#define vtk_m_cont_arg_TransportTagExecObject_h

#include <vtkm/Types.h>

#include <vtkm/cont/arg/Transport.h>

#include <vtkm/cont/ExecutionObjectBase.h>


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
  // If you get a compile error here, it means you tried to use an object that is not an execution
  // object as an argument that is expected to be one. All execution objects are expected to
  // inherit from vtkm::cont::ExecutionObjectBase and have a PrepareForExecution method.
  VTKM_IS_EXECUTION_OBJECT(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForExecution(Device()));
  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(ContObjectType& object, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    return object.PrepareForExecution(Device());
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagExecObject_h
