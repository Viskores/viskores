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
#ifndef vtk_m_cont_arg_TransportTagCellSetIn_h
#define vtk_m_cont_arg_TransportTagCellSetIn_h

#include <vtkm/Types.h>

#include <vtkm/cont/CellSet.h>

#include <vtkm/cont/arg/Transport.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagCellSetIn is a tag used with the \c Transport class to
/// transport topology objects for input data.
///
template <typename FromTopology, typename ToTopology>
struct TransportTagCellSetIn
{
};

template <typename FromTopology, typename ToTopology, typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagCellSetIn<FromTopology, ToTopology>,
                 ContObjectType,
                 Device>
{
  VTKM_IS_CELL_SET(ContObjectType);

  using ExecObjectType = decltype(
    std::declval<ContObjectType>().PrepareForInput(Device(), FromTopology(), ToTopology()));

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(const ContObjectType& object, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    return object.PrepareForInput(Device(), FromTopology(), ToTopology());
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagCellSetIn_h
