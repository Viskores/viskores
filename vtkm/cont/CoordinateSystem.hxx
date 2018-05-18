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
#ifndef vtk_m_cont_CoordinateSystem_hxx
#define vtk_m_cont_CoordinateSystem_hxx

#include <vtkm/cont/CoordinateSystem.h>

namespace vtkm
{
namespace cont
{
namespace detail
{

struct MakeArrayHandleVirtualCoordinatesFunctor
{
  template <typename StorageTag>
  VTKM_CONT void operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>, StorageTag>& array,
    ArrayHandleVirtualCoordinates& output) const
  {
    output = vtkm::cont::ArrayHandleVirtualCoordinates(array);
  }

  template <typename StorageTag>
  VTKM_CONT void operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>, StorageTag>& array,
    ArrayHandleVirtualCoordinates& output) const
  {
    output = vtkm::cont::ArrayHandleVirtualCoordinates(array);
  }
};

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandleVirtualCoordinates MakeArrayHandleVirtualCoordinates(
  const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& array)
{
  vtkm::cont::ArrayHandleVirtualCoordinates output;
  vtkm::cont::CastAndCall(array.ResetTypeList(vtkm::TypeListTagFieldVec3{}),
                          MakeArrayHandleVirtualCoordinatesFunctor{},
                          output);
  return output;
}
} // namespace detail

template <typename TypeList, typename StorageList>
VTKM_CONT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& data)
  : Superclass(name, Association::POINTS, detail::MakeArrayHandleVirtualCoordinates(data))
{
}

template <typename T, typename Storage>
VTKM_CONT CoordinateSystem::CoordinateSystem(std::string name,
                                             const vtkm::cont::ArrayHandle<T, Storage>& data)
  : Superclass(name, Association::POINTS, vtkm::cont::ArrayHandleVirtualCoordinates(data))
{
}

template <typename T, typename Storage>
VTKM_CONT void CoordinateSystem::SetData(const vtkm::cont::ArrayHandle<T, Storage>& newdata)
{
  this->SetData(vtkm::cont::ArrayHandleVirtualCoordinates(newdata));
}

template <typename TypeList, typename StorageList>
VTKM_CONT void CoordinateSystem::SetData(
  const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& newdata)
{
  this->SetData(detail::MakeArrayHandleVirtualCoordinates(newdata));
}
}
}
#endif
