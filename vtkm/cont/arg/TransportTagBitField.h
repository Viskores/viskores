//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagBitField_h
#define vtk_m_cont_arg_TransportTagBitField_h

#include <vtkm/cont/arg/Transport.h>

#include <vtkm/cont/BitField.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

struct TransportTagBitFieldIn
{
};
struct TransportTagBitFieldOut
{
};
struct TransportTagBitFieldInOut
{
};

template <typename Device>
struct Transport<vtkm::cont::arg::TransportTagBitFieldIn, vtkm::cont::BitField, Device>
{
  using ExecObjectType =
    typename vtkm::cont::BitField::template ExecutionTypes<Device>::PortalConst;

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(vtkm::cont::BitField& field, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    return field.PrepareForInput(Device{});
  }
};

template <typename Device>
struct Transport<vtkm::cont::arg::TransportTagBitFieldOut, vtkm::cont::BitField, Device>
{
  using ExecObjectType = typename vtkm::cont::BitField::template ExecutionTypes<Device>::Portal;

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(vtkm::cont::BitField& field, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    // This behaves similarly to WholeArray tags, where "Out" maps to InPlace
    // since we don't want to reallocate or enforce size restrictions.
    return field.PrepareForInPlace(Device{});
  }
};

template <typename Device>
struct Transport<vtkm::cont::arg::TransportTagBitFieldInOut, vtkm::cont::BitField, Device>
{
  using ExecObjectType = typename vtkm::cont::BitField::template ExecutionTypes<Device>::Portal;

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(vtkm::cont::BitField& field, const InputDomainType&, vtkm::Id, vtkm::Id) const
  {
    return field.PrepareForInPlace(Device{});
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagBitField_h
