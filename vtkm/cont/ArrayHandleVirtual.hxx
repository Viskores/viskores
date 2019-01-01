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
#include <vtkm/cont/StorageAny.hxx>
#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace cont
{

template <typename T>
template <typename ArrayHandleType>
ArrayHandleType inline ArrayHandle<T, StorageTagVirtual>::CastToType(
  std::true_type vtkmNotUsed(valueTypesMatch),
  std::false_type vtkmNotUsed(notFromArrayHandleVirtual)) const
{
  if (!this->Storage)
  {
    VTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", vtkm::cont::TypeName<ArrayHandleType>());
  }
  using S = typename ArrayHandleType::StorageTag;
  const auto* any = this->Storage->template Cast<vtkm::cont::StorageAny<T, S>>();
  return any->GetHandle();
}
}
} // namespace vtkm::const


#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>

//=============================================================================
// Specializations of serialization related classes
namespace diy
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleVirtual<T>>
{

  static VTKM_CONT void save(diy::BinaryBuffer& bb, const vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    vtkm::cont::ArrayHandle<T> array;
    diy::load(bb, array);
    obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
  }
};

template <typename T>
struct IntAnySerializer
{
  using CountingType = vtkm::cont::ArrayHandleCounting<T>;
  using ConstantType = vtkm::cont::ArrayHandleConstant<T>;
  using BasicType = vtkm::cont::ArrayHandle<T>;

  static VTKM_CONT void save(diy::BinaryBuffer& bb, const vtkm::cont::ArrayHandleVirtual<T>& obj)
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

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    std::string typeString;
    diy::load(bb, typeString);

    if (typeString == vtkm::cont::TypeString<CountingType>::Get())
    {
      CountingType array;
      diy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
    }
    else if (typeString == vtkm::cont::TypeString<ConstantType>::Get())
    {
      ConstantType array;
      diy::load(bb, array);
      obj = std::move(vtkm::cont::ArrayHandleVirtual<T>{ array });
    }
    else
    {
      vtkm::cont::ArrayHandle<T> array;
      diy::load(bb, array);
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
}

#endif
