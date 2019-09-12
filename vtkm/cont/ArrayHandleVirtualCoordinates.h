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

namespace vtkm
{
namespace cont
{

/// ArrayHandleVirtualCoordinates is a specialization of ArrayHandle.
class VTKM_ALWAYS_EXPORT ArrayHandleVirtualCoordinates final
  : public vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>
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

} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

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
      vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<BasicCoordsType>::Get());
      vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
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

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleVirtualCoordinates_h
