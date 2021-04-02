//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleVirtualCoordinates_h
#define vtk_m_cont_ArrayHandleVirtualCoordinates_h

#include <vtkm/cont/ArrayHandleVirtual.h>

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Logging.h>

#include <memory>
#include <type_traits>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "ArrayHandleVirtualCoordiantes is removed. Do not include ArrayHandleVirtualCoordinates.h"
#endif

namespace vtkm
{
namespace cont
{

VTKM_DEPRECATED_SUPPRESS_BEGIN

/// ArrayHandleVirtualCoordinates is a specialization of ArrayHandle.
class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Virtual ArrayHandles are being phased out.")
  ArrayHandleVirtualCoordinates final : public vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleVirtualCoordinates,
                                (vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>));

  template <typename T, typename S>
  explicit ArrayHandleVirtualCoordinates(const vtkm::cont::ArrayHandle<T, S>& ah)
    : vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>(vtkm::cont::make_ArrayHandleCast<ValueType>(ah))
  {
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
struct SerializableTypeString<vtkm::cont::ArrayHandleVirtualCoordinates>
{
  static VTKM_CONT const std::string Get() { return "AH_VirtualCoordinates"; }
};

VTKM_DEPRECATED_SUPPRESS_END

} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

VTKM_DEPRECATED_SUPPRESS_BEGIN

template <>
struct Serialization<vtkm::cont::ArrayHandleVirtualCoordinates>
{
private:
  using Type = vtkm::cont::ArrayHandleVirtualCoordinates;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

  using BasicCoordsType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using RectilinearCoordsArrayType =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& baseObj)
  {
    Type obj(baseObj);
    const vtkm::cont::internal::detail::StorageVirtual* storage =
      obj.GetStorage().GetStorageVirtual();
    if (obj.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      using HandleType = vtkm::cont::ArrayHandleUniformPointCoordinates;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      auto array = storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<HandleType>::Get());
      vtkmdiy::save(bb, array->GetHandle());
    }
    else if (obj.IsType<RectilinearCoordsArrayType>())
    {
      using HandleType = RectilinearCoordsArrayType;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      auto array = storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<HandleType>::Get());
      vtkmdiy::save(bb, array->GetHandle());
    }
    else
    {
      using HandleType = BasicCoordsType;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      HandleType array;
      if (obj.IsType<BasicCoordsType>())
      {
        // If the object actually is a BasicCoordsType, just save it.
        array =
          storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>()->GetHandle();
      }
      else
      {
        // Give up and deep copy data.
        vtkm::Id size = obj.GetNumberOfValues();
        array.Allocate(size);
        auto src = obj.ReadPortal();
        auto dest = array.WritePortal();
        for (vtkm::IdComponent index = 0; index < size; ++index)
        {
          dest.Set(index, src.Get(index));
        }
      }
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<BasicCoordsType>::Get());
      vtkmdiy::save(bb, array);
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    std::string typeString;
    vtkmdiy::load(bb, typeString);

    if (typeString ==
        vtkm::cont::SerializableTypeString<vtkm::cont::ArrayHandleUniformPointCoordinates>::Get())
    {
      vtkm::cont::ArrayHandleUniformPointCoordinates array;
      vtkmdiy::load(bb, array);
      obj = vtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else if (typeString == vtkm::cont::SerializableTypeString<RectilinearCoordsArrayType>::Get())
    {
      RectilinearCoordsArrayType array;
      vtkmdiy::load(bb, array);
      obj = vtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else if (typeString == vtkm::cont::SerializableTypeString<BasicCoordsType>::Get())
    {
      BasicCoordsType array;
      vtkmdiy::load(bb, array);
      obj = vtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else
    {
      throw vtkm::cont::ErrorBadType(
        "Error deserializing ArrayHandleVirtualCoordinates. TypeString: " + typeString);
    }
  }
};

VTKM_DEPRECATED_SUPPRESS_END

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleVirtualCoordinates_h
