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
#ifndef vtk_m_cont_ArrayHandleVirtualCoordinates_h
#define vtk_m_cont_ArrayHandleVirtualCoordinates_h

#include <vtkm/cont/ArrayHandleAny.h>
#include <vtkm/cont/ArrayHandleVirtual.h>

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/TryExecute.h>

#include <memory>
#include <type_traits>

namespace vtkm
{
namespace cont
{

/// ArrayHandleVirtualCoordinates is a specialization of ArrayHandle.
class VTKM_CONT_EXPORT ArrayHandleVirtualCoordinates final
  : public vtkm::cont::ArrayHandleVirtual<vtkm::Vec<vtkm::FloatDefault, 3>>
{
public:
  using ValueType = vtkm::Vec<vtkm::FloatDefault, 3>;
  using StorageTag = vtkm::cont::StorageTagVirtual;

  using NonDefaultCoord = typename std::conditional<std::is_same<vtkm::FloatDefault, float>::value,
                                                    vtkm::Vec<double, 3>,
                                                    vtkm::Vec<float, 3>>::type;

  ArrayHandleVirtualCoordinates()
    : vtkm::cont::ArrayHandleVirtual<ValueType>()
  {
  }

  explicit ArrayHandleVirtualCoordinates(
    const vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagVirtual>& ah)
    : vtkm::cont::ArrayHandleVirtual<ValueType>(ah)
  {
  }

  template <typename S>
  explicit ArrayHandleVirtualCoordinates(const vtkm::cont::ArrayHandle<ValueType, S>& ah)
    : vtkm::cont::ArrayHandleVirtual<ValueType>(
        std::make_shared<vtkm::cont::StorageAny<ValueType, S>>(ah))
  {
  }

  template <typename S>
  explicit ArrayHandleVirtualCoordinates(const vtkm::cont::ArrayHandle<NonDefaultCoord, S>& ah)
    : vtkm::cont::ArrayHandleVirtual<ValueType>()
  {
    auto castedHandle = vtkm::cont::make_ArrayHandleCast<ValueType>(ah);
    using ST = typename decltype(castedHandle)::StorageTag;
    this->Storage = std::make_shared<vtkm::cont::StorageAny<ValueType, ST>>(castedHandle);
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT ArrayHandleType Cast() const
  {
    using T = typename ArrayHandleType::ValueType;
    using S = typename ArrayHandleType::StorageTag;
    const vtkm::cont::StorageVirtual* storage = this->GetStorage();
    const auto* any = storage->Cast<vtkm::cont::StorageAny<T, S>>();
    return any->GetHandle();
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
                 Functor&& f,
                 Args&&... args)
{
  using HandleType = ArrayHandleUniformPointCoordinates;
  if (coords.IsType<HandleType>())
  {
    HandleType uniform = coords.Cast<HandleType>();
    f(uniform, std::forward<Args>(args)...);
  }
  else
  {
    f(coords, std::forward<Args>(args)...);
  }
}



template <>
struct TypeString<vtkm::cont::ArrayHandleVirtualCoordinates>
{
  static VTKM_CONT const std::string Get() { return "AH_VirtualCoordinates"; }
};

} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
namespace diy
{

template <>
struct Serialization<vtkm::cont::ArrayHandleVirtualCoordinates>
{
private:
  using Type = vtkm::cont::ArrayHandleVirtualCoordinates;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

  using BasicCoordsType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;
  using RectilinearCoordsArrayType =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    const vtkm::cont::StorageVirtual* storage = obj.GetStorage();
    if (obj.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      using HandleType = vtkm::cont::ArrayHandleUniformPointCoordinates;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      auto array = storage->Cast<vtkm::cont::StorageAny<T, S>>();
      diy::save(bb, vtkm::cont::TypeString<HandleType>::Get());
      diy::save(bb, array->GetHandle());
    }
    else if (obj.IsType<RectilinearCoordsArrayType>())
    {
      using HandleType = RectilinearCoordsArrayType;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      auto array = storage->Cast<vtkm::cont::StorageAny<T, S>>();
      diy::save(bb, vtkm::cont::TypeString<HandleType>::Get());
      diy::save(bb, array->GetHandle());
    }
    else
    {
      diy::save(bb, vtkm::cont::TypeString<BasicCoordsType>::Get());
      vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    std::string typeString;
    diy::load(bb, typeString);

    if (typeString == vtkm::cont::TypeString<vtkm::cont::ArrayHandleUniformPointCoordinates>::Get())
    {
      vtkm::cont::ArrayHandleUniformPointCoordinates array;
      diy::load(bb, array);
      obj = vtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else if (typeString == vtkm::cont::TypeString<RectilinearCoordsArrayType>::Get())
    {
      RectilinearCoordsArrayType array;
      diy::load(bb, array);
      obj = vtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else if (typeString == vtkm::cont::TypeString<BasicCoordsType>::Get())
    {
      BasicCoordsType array;
      diy::load(bb, array);
      obj = vtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else
    {
      throw vtkm::cont::ErrorBadType(
        "Error deserializing ArrayHandleVirtualCoordinates. TypeString: " + typeString);
    }
  }
};

} // diy

#endif // vtk_m_cont_ArrayHandleVirtualCoordinates_h
