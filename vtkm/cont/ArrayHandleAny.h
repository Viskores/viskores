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
#ifndef vtk_m_cont_ArrayHandleAny_h
#define vtk_m_cont_ArrayHandleAny_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtual.h>

namespace vtkm
{
namespace cont
{
template <typename T, typename S>
class VTKM_ALWAYS_EXPORT StorageAny final : public vtkm::cont::StorageVirtual
{
public:
  VTKM_CONT
  StorageAny(const vtkm::cont::ArrayHandle<T, S>& ah);

  VTKM_CONT
  ~StorageAny() = default;

  const vtkm::cont::ArrayHandle<T, S>& GetHandle() const { return this->Handle; }

  vtkm::Id GetNumberOfValues() const { return this->Handle.GetNumberOfValues(); }

  void ReleaseResourcesExecution();
  void ReleaseResources();

private:
  // StorageAny is meant to be seamless when it comes to IsType so we will match
  // when either the type_info is 'StorageAny<T,S>' or 'Storage<T,S>'. That is why
  // we need to override the default implementation.
  bool IsSameType(const std::type_info& other) const
  {
    //We don't wan to check just 'S' as that just the tag
    using ST = typename vtkm::cont::internal::Storage<T, S>;
    return other == typeid(ST) || other == typeid(*this);
  }

  std::unique_ptr<StorageVirtual> MakeNewInstance() const
  {
    return std::unique_ptr<StorageVirtual>(new StorageAny<T, S>{ vtkm::cont::ArrayHandle<T, S>{} });
  }


  void ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const;
  void ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload);

  void TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                              vtkm::cont::DeviceAdapterId devId) const;

  void TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                               vtkm::Id numberOfValues,
                               vtkm::cont::DeviceAdapterId devId);


  vtkm::cont::ArrayHandle<T, S> Handle;
};

/// ArrayHandleAny is a specialization of ArrayHandle.
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleAny final : public vtkm::cont::ArrayHandleVirtual<T>
{
public:
  template <typename S>
  VTKM_CONT ArrayHandleAny(const vtkm::cont::ArrayHandle<T, S>& ah)
    : vtkm::cont::ArrayHandleVirtual<T>(std::make_shared<StorageAny<T, S>>(ah))
  {
  }

  ~ArrayHandleAny() = default;
};

/// A convenience function for creating an ArrayHandleAny.
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandleAny<T> make_ArrayHandleAny(const vtkm::cont::ArrayHandle<T>& ah)
{
  return vtkm::cont::ArrayHandleAny<T>(ah);
}


template <typename Functor, typename... Args>
void CastAndCall(vtkm::cont::ArrayHandleVirtual<vtkm::Vec<vtkm::FloatDefault, 3>> coords,
                 Functor&& f,
                 Args&&... args)
{
  using HandleType = ArrayHandleUniformPointCoordinates;
  using T = typename HandleType::ValueType;
  using S = typename HandleType::StorageTag;
  if (coords.IsType<HandleType>())
  {
    const vtkm::cont::StorageVirtual* storage = coords.GetStorage();
    auto* any = storage->Cast<vtkm::cont::StorageAny<T, S>>();
    f(any->GetHandle(), std::forward<Args>(args)...);
  }
  else
  {
    f(coords, std::forward<Args>(args)...);
  }
}
}
} //namespace vtkm::cont



#endif //vtk_m_cont_ArrayHandleAny_h
