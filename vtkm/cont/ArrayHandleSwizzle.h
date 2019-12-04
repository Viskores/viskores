//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleSwizzle_h
#define vtk_m_cont_ArrayHandleSwizzle_h

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <sstream>

namespace vtkm
{
namespace cont
{

template <typename InVecType, vtkm::IdComponent OutVecSize>
struct ResizeVectorType
{
private:
  using ComponentType = typename vtkm::VecTraits<InVecType>::ComponentType;

public:
  using Type = vtkm::Vec<ComponentType, OutVecSize>;
};

template <typename ArrayHandleType, vtkm::IdComponent OutVecSize>
class StorageTagSwizzle
{
};

namespace internal
{

template <typename ArrayHandleType, vtkm::IdComponent OutputSize>
struct ArrayHandleSwizzleTraits;

template <typename V, vtkm::IdComponent C, typename S, vtkm::IdComponent OutSize>
struct ArrayHandleSwizzleTraits<vtkm::cont::ArrayHandle<vtkm::Vec<V, C>, S>, OutSize>
{
  using ComponentType = V;
  static constexpr vtkm::IdComponent InVecSize = C;
  static constexpr vtkm::IdComponent OutVecSize = OutSize;

  VTKM_STATIC_ASSERT(OutVecSize <= InVecSize);
  static constexpr bool AllCompsUsed = (InVecSize == OutVecSize);

  using InValueType = vtkm::Vec<ComponentType, InVecSize>;
  using OutValueType = vtkm::Vec<ComponentType, OutVecSize>;

  using InStorageTag = S;
  using InArrayHandleType = vtkm::cont::ArrayHandle<InValueType, InStorageTag>;

  using OutStorageTag = vtkm::cont::StorageTagSwizzle<InArrayHandleType, OutVecSize>;
  using OutArrayHandleType = vtkm::cont::ArrayHandle<OutValueType, OutStorageTag>;

  using InStorageType = vtkm::cont::internal::Storage<InValueType, InStorageTag>;
  using OutStorageType = vtkm::cont::internal::Storage<OutValueType, OutStorageTag>;

  using MapType = vtkm::Vec<vtkm::IdComponent, OutVecSize>;

  VTKM_CONT
  static void ValidateMap(const MapType& map)
  {
    for (vtkm::IdComponent i = 0; i < OutVecSize; ++i)
    {
      if (map[i] < 0 || map[i] >= InVecSize)
      {
        std::ostringstream error;
        error << "Invalid swizzle map: Element " << i << " (" << map[i]
              << ") outside valid range [0, " << InVecSize << ").";
        throw vtkm::cont::ErrorBadValue(error.str());
      }
      for (vtkm::IdComponent j = i + 1; j < OutVecSize; ++j)
      {
        if (map[i] == map[j])
        {
          std::ostringstream error;
          error << "Invalid swizzle map: Repeated element (" << map[i] << ")"
                << " at indices " << i << " and " << j << ".";
          throw vtkm::cont::ErrorBadValue(error.str());
        }
      }
    }
  }

  VTKM_EXEC_CONT
  static void Swizzle(const InValueType& in, OutValueType& out, const MapType& map)
  {
    for (vtkm::IdComponent i = 0; i < OutSize; ++i)
    {
      out[i] = in[map[i]];
    }
  }

  VTKM_EXEC_CONT
  static void UnSwizzle(const OutValueType& out, InValueType& in, const MapType& map)
  {
    for (vtkm::IdComponent i = 0; i < OutSize; ++i)
    {
      in[map[i]] = out[i];
    }
  }
};

template <typename PortalType, typename ArrayHandleType, vtkm::IdComponent OutSize>
class VTKM_ALWAYS_EXPORT ArrayPortalSwizzle
{
  using Traits = internal::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;
  using Writable = vtkm::internal::PortalSupportsSets<PortalType>;

public:
  using MapType = typename Traits::MapType;
  using ValueType = typename Traits::OutValueType;

  VTKM_EXEC_CONT
  ArrayPortalSwizzle()
    : Portal()
    , Map()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalSwizzle(const PortalType& portal, const MapType& map)
    : Portal(portal)
    , Map(map)
  {
  }

  // Copy constructor
  VTKM_EXEC_CONT ArrayPortalSwizzle(const ArrayPortalSwizzle& src)
    : Portal(src.GetPortal())
    , Map(src.GetMap())
  {
  }

  ArrayPortalSwizzle& operator=(const ArrayPortalSwizzle& src) = default;
  ArrayPortalSwizzle& operator=(ArrayPortalSwizzle&& src) = default;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    ValueType result;
    Traits::Swizzle(this->Portal.Get(index), result, this->Map);
    return result;
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    if (Traits::AllCompsUsed)
    { // No need to prefetch the value, all values overwritten
      typename Traits::InValueType tmp;
      Traits::UnSwizzle(value, tmp, this->Map);
      this->Portal.Set(index, tmp);
    }
    else
    { // Not all values used -- need to initialize the vector
      typename Traits::InValueType tmp = this->Portal.Get(index);
      Traits::UnSwizzle(value, tmp, this->Map);
      this->Portal.Set(index, tmp);
    }
  }

  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

  VTKM_EXEC_CONT
  const MapType& GetMap() const { return this->Map; }

private:
  PortalType Portal;
  MapType Map;
};

template <typename ArrayHandleType, vtkm::IdComponent OutSize>
class Storage<typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
              vtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>>
{
  using Traits = internal::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;

public:
  using PortalType =
    ArrayPortalSwizzle<typename ArrayHandleType::PortalControl, ArrayHandleType, OutSize>;
  using PortalConstType =
    ArrayPortalSwizzle<typename ArrayHandleType::PortalConstControl, ArrayHandleType, OutSize>;
  using MapType = typename Traits::MapType;
  using ValueType = typename Traits::OutValueType;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& array, const MapType& map)
    : Array(array)
    , Map(map)
    , Valid(true)
  {
    Traits::ValidateMap(this->Map);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(), this->Map);
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(), this->Map);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Array.Allocate(numberOfValues);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Array.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    this->Array.ReleaseResources();
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

  VTKM_CONT
  const MapType& GetMap() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Map;
  }

private:
  ArrayHandleType Array;
  MapType Map;
  bool Valid;
};

template <typename ArrayHandleType, vtkm::IdComponent OutSize, typename DeviceTag>
class ArrayTransfer<typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
                    vtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>,
                    DeviceTag>
{
  using InExecTypes = typename ArrayHandleType::template ExecutionTypes<DeviceTag>;
  using Traits = ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;
  using StorageType = typename Traits::OutStorageType;
  using MapType = typename Traits::MapType;

  template <typename InPortalT>
  using OutExecType = ArrayPortalSwizzle<InPortalT, ArrayHandleType, OutSize>;

public:
  using ValueType = typename Traits::OutValueType;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = OutExecType<typename InExecTypes::Portal>;
  using PortalConstExecution = OutExecType<typename InExecTypes::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Map(storage->GetMap())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(DeviceTag()), this->Map);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(DeviceTag()), this->Map);
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, DeviceTag()), this->Map);
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }


private:
  ArrayHandleType Array;
  MapType Map;
};

} // end namespace internal

template <typename ArrayHandleType, vtkm::IdComponent OutSize>
class ArrayHandleSwizzle
  : public ArrayHandle<
      typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
      vtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>>
{
public:
  using SwizzleTraits = internal::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;
  using StorageType = typename SwizzleTraits::OutStorageType;
  using MapType = typename SwizzleTraits::MapType;

  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleSwizzle,
    (ArrayHandleSwizzle<ArrayHandleType, OutSize>),
    (ArrayHandle<typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
                 vtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>>));

  VTKM_CONT
  ArrayHandleSwizzle(const ArrayHandleType& array, const MapType& map)
    : Superclass(StorageType(array, map))
  {
  }
};

template <typename ArrayHandleType, vtkm::IdComponent OutSize>
VTKM_CONT ArrayHandleSwizzle<ArrayHandleType, OutSize> make_ArrayHandleSwizzle(
  const ArrayHandleType& array,
  const vtkm::Vec<vtkm::IdComponent, OutSize>& map)
{
  return ArrayHandleSwizzle<ArrayHandleType, OutSize>(array, map);
}

template <typename ArrayHandleType, typename... SwizzleIndexTypes>
VTKM_CONT ArrayHandleSwizzle<ArrayHandleType, vtkm::IdComponent(sizeof...(SwizzleIndexTypes) + 1)>
make_ArrayHandleSwizzle(const ArrayHandleType& array,
                        vtkm::IdComponent swizzleIndex0,
                        SwizzleIndexTypes... swizzleIndices)
{
  return make_ArrayHandleSwizzle(array, vtkm::make_Vec(swizzleIndex0, swizzleIndices...));
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename AH, vtkm::IdComponent NComps>
struct SerializableTypeString<vtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_Swizzle<" + SerializableTypeString<AH>::Get() + "," + std::to_string(NComps) + ">";
    return name;
  }
};

template <typename AH, vtkm::IdComponent NComps>
struct SerializableTypeString<vtkm::cont::ArrayHandle<
  vtkm::Vec<typename vtkm::VecTraits<typename AH::ValueType>::ComponentType, NComps>,
  vtkm::cont::StorageTagSwizzle<AH, NComps>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, vtkm::IdComponent NComps>
struct Serialization<vtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
private:
  using Type = vtkm::cont::ArrayHandleSwizzle<AH, NComps>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetArray());
    vtkmdiy::save(bb, storage.GetMap());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);
    vtkm::Vec<vtkm::IdComponent, NComps> map;
    vtkmdiy::load(bb, map);
    obj = vtkm::cont::make_ArrayHandleSwizzle(array, map);
  }
};

template <typename AH, vtkm::IdComponent NComps>
struct Serialization<vtkm::cont::ArrayHandle<
  vtkm::Vec<typename vtkm::VecTraits<typename AH::ValueType>::ComponentType, NComps>,
  vtkm::cont::StorageTagSwizzle<AH, NComps>>>
  : Serialization<vtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleSwizzle_h
