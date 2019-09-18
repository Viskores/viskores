//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagArrayIn_h
#define vtk_m_cont_arg_TransportTagArrayIn_h

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/arg/Transport.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagArrayIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data.
///
struct TransportTagArrayIn
{
};

template <typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagArrayIn, ContObjectType, Device>
{
  VTKM_IS_ARRAY_HANDLE(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(Device()));

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType operator()(const ContObjectType& object,
                                      const InputDomainType& vtkmNotUsed(inputDomain),
                                      vtkm::Id inputRange,
                                      vtkm::Id vtkmNotUsed(outputRange)) const
  {
    if (object.GetNumberOfValues() != inputRange)
    {
      throw vtkm::cont::ErrorBadValue("Input array to worklet invocation the wrong size.");
    }

    return object.PrepareForInput(Device());
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagArrayIn_h
