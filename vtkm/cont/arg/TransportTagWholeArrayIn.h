//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagWholeArrayIn_h
#define vtk_m_cont_arg_TransportTagWholeArrayIn_h

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/arg/Transport.h>

#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for in-place arrays with random access.
///
/// \c TransportTagWholeArrayIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data.
///
/// The worklet will have random access to the array through a portal
/// interface.
///
struct TransportTagWholeArrayIn
{
};

template <typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagWholeArrayIn, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that
  // is not an array handle as an argument that is expected to be one.
  VTKM_IS_ARRAY_HANDLE(ContObjectType);

  using ValueType = typename ContObjectType::ValueType;
  using StorageTag = typename ContObjectType::StorageTag;

  using ExecObjectType = vtkm::exec::ExecutionWholeArrayConst<ValueType, StorageTag, Device>;

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(ContObjectType& array, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.

    return ExecObjectType(array);
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagWholeArrayIn_h
