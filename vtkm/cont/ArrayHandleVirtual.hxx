//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleVirtual_hxx
#define vtk_m_cont_ArrayHandleVirtual_hxx

#include <vtkm/cont/ArrayHandleVirtual.h>
#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace cont
{

template <typename T>
template <typename ArrayHandleType>
ArrayHandleType inline ArrayHandleVirtual<T>::CastToType(
  std::true_type vtkmNotUsed(valueTypesMatch),
  std::false_type vtkmNotUsed(notFromArrayHandleVirtual)) const
{
  auto* storage = this->GetStorage().GetStorageVirtual();
  if (!storage)
  {
    VTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", vtkm::cont::TypeToString<ArrayHandleType>());
  }
  using S = typename ArrayHandleType::StorageTag;
  const auto* castStorage =
    storage->template Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
  return castStorage->GetHandle();
}
}
} // namespace vtkm::cont


#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleVirtual<T>>
{

  static VTKM_CONT void save(vtkmdiy::BinaryBuffer& bb,
                             const vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    vtkm::cont::ArrayHandle<T> array;
    vtkmdiy::load(bb, array);
    obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
  }
};

template <typename T>
struct IntAnySerializer
{
  using CountingType = vtkm::cont::ArrayHandleCounting<T>;
  using ConstantType = vtkm::cont::ArrayHandleConstant<T>;
  using BasicType = vtkm::cont::ArrayHandle<T>;

  static VTKM_CONT void save(vtkmdiy::BinaryBuffer& bb,
                             const vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    if (obj.template IsType<CountingType>())
    {
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<CountingType>::Get());

      using S = typename CountingType::StorageTag;
      const vtkm::cont::internal::detail::StorageVirtual* storage =
        obj.GetStorage().GetStorageVirtual();
      auto* castStorage = storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      vtkmdiy::save(bb, castStorage->GetHandle());
    }
    else if (obj.template IsType<ConstantType>())
    {
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<ConstantType>::Get());

      using S = typename ConstantType::StorageTag;
      const vtkm::cont::internal::detail::StorageVirtual* storage =
        obj.GetStorage().GetStorageVirtual();
      auto* castStorage = storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      vtkmdiy::save(bb, castStorage->GetHandle());
    }
    else
    {
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<BasicType>::Get());
      vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    std::string typeString;
    vtkmdiy::load(bb, typeString);

    if (typeString == vtkm::cont::SerializableTypeString<CountingType>::Get())
    {
      CountingType array;
      vtkmdiy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
    }
    else if (typeString == vtkm::cont::SerializableTypeString<ConstantType>::Get())
    {
      ConstantType array;
      vtkmdiy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
    }
    else
    {
      vtkm::cont::ArrayHandle<T> array;
      vtkmdiy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
    }
  }
};


template <>
struct Serialization<vtkm::cont::ArrayHandleVirtual<vtkm::UInt8>>
  : public IntAnySerializer<vtkm::UInt8>
{
};
template <>
struct Serialization<vtkm::cont::ArrayHandleVirtual<vtkm::Int32>>
  : public IntAnySerializer<vtkm::Int32>
{
};
template <>
struct Serialization<vtkm::cont::ArrayHandleVirtual<vtkm::Int64>>
  : public IntAnySerializer<vtkm::Int64>
{
};

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>>
  : public Serialization<vtkm::cont::ArrayHandleVirtual<T>>
{
};

} // mangled_diy_namespace

#endif
