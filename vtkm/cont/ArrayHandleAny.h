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
  std::unique_ptr<StorageVirtual> MakeNewInstance() const
  {
    return std::unique_ptr<StorageVirtual>(new StorageAny<T, S>{ vtkm::cont::ArrayHandle<T, S>{} });
  }


  void ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const;
  void ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload);

  void TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                              vtkm::cont::DeviceAdapterId devId) const;

  void TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                               OutputMode mode,
                               vtkm::Id numberOfValues,
                               vtkm::cont::DeviceAdapterId devId);

  vtkm::cont::ArrayHandle<T, S> Handle;
};

/// ArrayHandleAny is a specialization of ArrayHandle.
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleAny final : public vtkm::cont::ArrayHandleVirtual<T>
{
public:
  ///construct a valid ArrayHandleAny from an existing ArrayHandle
  template <typename S>
  VTKM_CONT ArrayHandleAny(const vtkm::cont::ArrayHandle<T, S>& ah)
    : vtkm::cont::ArrayHandleVirtual<T>(std::make_shared<StorageAny<T, S>>(ah))
  {
  }

  ///construct an invalid ArrayHandleAny that has a nullptr storage
  VTKM_CONT ArrayHandleAny()
    : vtkm::cont::ArrayHandleVirtual<T>()
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

//=============================================================================
// Specializations of serialization related classes
template <typename T>
struct TypeString<vtkm::cont::ArrayHandleAny<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Any<" + TypeString<T>::Get() + ">";
    return name;
  }
};
}
} //namespace vtkm::cont


#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>

//=============================================================================
// Specializations of serialization related classes
namespace diy
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleAny<T>>
{

  static VTKM_CONT void save(diy::BinaryBuffer& bb, const vtkm::cont::ArrayHandleAny<T>& obj)
  {
    vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandleAny<T>& obj)
  {
    vtkm::cont::ArrayHandle<T> array;
    diy::load(bb, array);
    obj = std::move(vtkm::cont::ArrayHandleAny<T>{ array });
  }
};

template <typename T>
struct IntAnySerializer
{
  using CountingType = vtkm::cont::ArrayHandleCounting<T>;
  using ConstantType = vtkm::cont::ArrayHandleConstant<T>;
  using BasicType = vtkm::cont::ArrayHandle<T>;

  static VTKM_CONT void save(diy::BinaryBuffer& bb, const vtkm::cont::ArrayHandleAny<T>& obj)
  {
    if (obj.template IsType<CountingType>())
    {
      diy::save(bb, vtkm::cont::TypeString<CountingType>::Get());

      using S = typename CountingType::StorageTag;
      const vtkm::cont::StorageVirtual* storage = obj.GetStorage();
      auto* any = storage->Cast<vtkm::cont::StorageAny<T, S>>();
      diy::save(bb, any->GetHandle());
    }
    else if (obj.template IsType<ConstantType>())
    {
      diy::save(bb, vtkm::cont::TypeString<ConstantType>::Get());

      using S = typename ConstantType::StorageTag;
      const vtkm::cont::StorageVirtual* storage = obj.GetStorage();
      auto* any = storage->Cast<vtkm::cont::StorageAny<T, S>>();
      diy::save(bb, any->GetHandle());
    }
    else
    {
      diy::save(bb, vtkm::cont::TypeString<BasicType>::Get());
      vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandleAny<T>& obj)
  {
    std::string typeString;
    diy::load(bb, typeString);

    if (typeString == vtkm::cont::TypeString<CountingType>::Get())
    {
      CountingType array;
      diy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleAny<T>{ array });
    }
    else if (typeString == vtkm::cont::TypeString<ConstantType>::Get())
    {
      ConstantType array;
      diy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleAny<T>{ array });
    }
    else
    {
      vtkm::cont::ArrayHandle<T> array;
      diy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleAny<T>{ array });
    }
  }
};


template <>
struct Serialization<vtkm::cont::ArrayHandleAny<vtkm::UInt8>> : public IntAnySerializer<vtkm::UInt8>
{
};
template <>
struct Serialization<vtkm::cont::ArrayHandleAny<vtkm::Int32>> : public IntAnySerializer<vtkm::Int32>
{
};
template <>
struct Serialization<vtkm::cont::ArrayHandleAny<vtkm::Int64>> : public IntAnySerializer<vtkm::Int64>
{
};
}
#endif //vtk_m_cont_ArrayHandleAny_h
