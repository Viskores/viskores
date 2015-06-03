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
#ifndef vtk_m_cont_arg_TransportTagArrayIn_h
#define vtk_m_cont_arg_TransportTagArrayIn_h

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/arg/Transport.h>

namespace vtkm {
namespace cont {
namespace arg {

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagArrayIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data.
///
struct TransportTagArrayIn {  };

template<typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagArrayIn, ContObjectType, Device>
{
  ///\todo: something equivalent to VTKM_IS_ARRAY_HANDLE(ContObjectType);

  typedef typename ContObjectType::template ExecutionTypes<Device>::PortalConst
      ExecObjectType;

  VTKM_CONT_EXPORT
  ExecObjectType operator()(const ContObjectType &object, vtkm::Id) const
  {
    return object.PrepareForInput(Device());
  }
};

}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagArrayIn_h
