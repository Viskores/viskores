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
#ifndef vtk_m_cont_ArrayHandleVirtual_hxx
#define vtk_m_cont_ArrayHandleVirtual_hxx

#include <vtkm/cont/ArrayHandleVirtual.h>

#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace cont
{
namespace detail
{
template <typename DerivedPortal>
struct TransferToDevice
{
  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  bool operator()(DeviceAdapterTag devId, Payload&& payload, Args&&... args) const
  {
    using TransferType = cont::internal::VirtualObjectTransfer<DerivedPortal, DeviceAdapterTag>;


    //construct all new transfer payload
    auto host = std::unique_ptr<DerivedPortal>(new DerivedPortal(std::forward<Args>(args)...));
    auto transfer = std::make_shared<TransferType>(host.get());
    auto device = transfer->PrepareForExecution(true);

    payload.updateDevice(devId, std::move(host), device, std::static_pointer_cast<void>(transfer));

    return true;
  }
};
}

template <typename DerivedPortal, typename... Args>
inline void make_transferToDevice(vtkm::cont::DeviceAdapterId devId, Args&&... args)
{
  vtkm::cont::TryExecuteOnDevice(
    devId, detail::TransferToDevice<DerivedPortal>{}, std::forward<Args>(args)...);
}

template <typename DerivedPortal, typename Payload, typename... Args>
inline void make_hostPortal(Payload&& payload, Args&&... args)
{
  auto host = std::unique_ptr<DerivedPortal>(new DerivedPortal(std::forward<Args>(args)...));
  payload.updateHost(std::move(host));
}
}
} // namespace vtkm::virts



#include <vtkm/cont/ArrayHandleAny.h>
//=============================================================================
// Specializations of serialization related classes
namespace diy
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleVirtual<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleVirtual<T>;
  using BaseType = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>;
  using BasicType = vtkm::cont::ArrayHandle<T>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    if (obj.template IsType<vtkm::cont::ArrayHandleAny<T>>())
    {
      const auto& array = static_cast<const vtkm::cont::ArrayHandleAny<T>&>(obj);
      diy::save(bb, vtkm::cont::TypeString<vtkm::cont::ArrayHandleAny<T>>::Get());
      diy::save(bb, array);
    }
    else
    {
      diy::save(bb, vtkm::cont::TypeString<BasicType>::Get());
      vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    std::string typeString;
    diy::load(bb, typeString);

    if (typeString == vtkm::cont::TypeString<vtkm::cont::ArrayHandleAny<T>>::Get())
    {
      vtkm::cont::ArrayHandleAny<T> array;
      diy::load(bb, array);
      obj = std::move(array);
    }
    else if (typeString == vtkm::cont::TypeString<BasicType>::Get())
    {
      BasicType array;
      diy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleAny<T>{ array });
    }
    else
    {
      throw vtkm::cont::ErrorBadType("Error deserializing ArrayHandleVirtual. TypeString: " +
                                     typeString);
    }
  }
};
}


#endif
