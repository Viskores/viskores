//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<vtkm::Vec3f_32, StorageTag>& array,
                            ArrayHandleVirtualCoordinates& output) const
  {
    output = vtkm::cont::ArrayHandleVirtualCoordinates(array);
  }

  template <typename StorageTag>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<vtkm::Vec3f_64, StorageTag>& array,
                            ArrayHandleVirtualCoordinates& output) const
  {
    output = vtkm::cont::ArrayHandleVirtualCoordinates(array);
  }
};

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandleVirtualCoordinates MakeArrayHandleVirtualCoordinates(
  const vtkm::cont::VariantArrayHandleBase<TypeList>& array)
{
  vtkm::cont::ArrayHandleVirtualCoordinates output;
  vtkm::cont::CastAndCall(array.ResetTypes(vtkm::TypeListFieldVec3{}),
                          MakeArrayHandleVirtualCoordinatesFunctor{},
                          output);
  return output;
}
} // namespace detail

template <typename TypeList>
VTKM_CONT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::VariantArrayHandleBase<TypeList>& data)
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

template <typename TypeList>
VTKM_CONT void CoordinateSystem::SetData(
  const vtkm::cont::VariantArrayHandleBase<TypeList>& newdata)
{
  this->SetData(detail::MakeArrayHandleVirtualCoordinates(newdata));
}
}
}
#endif
