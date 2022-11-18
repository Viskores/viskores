//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_ArrayHandleReverse_h
#define vtk_m_cont_ArrayHandleReverse_h

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename PortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalReverse
{
  using Writable = vtkm::internal::PortalSupportsSets<PortalType>;

public:
  using ValueType = typename PortalType::ValueType;

  VTKM_EXEC_CONT
  ArrayPortalReverse()
    : portal()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalReverse(const PortalType& p)
    : portal(p)
  {
  }

  template <typename OtherPortal>
  VTKM_EXEC_CONT ArrayPortalReverse(const ArrayPortalReverse<OtherPortal>& src)
    : portal(src.GetPortal())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return this->portal.Get(portal.GetNumberOfValues() - index - 1);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->portal.Set(portal.GetNumberOfValues() - index - 1, value);
  }

private:
  PortalType portal;
};
}

template <typename StorageTag>
class VTKM_ALWAYS_EXPORT StorageTagReverse
{
};

namespace internal
{

template <typename T, typename ST>
class Storage<T, StorageTagReverse<ST>>
{
  using SourceStorage = Storage<T, ST>;

public:
  using ArrayHandleType = vtkm::cont::ArrayHandle<T, ST>;
  using ReadPortalType = ArrayPortalReverse<typename ArrayHandleType::ReadPortalType>;
  using WritePortalType = ArrayPortalReverse<typename ArrayHandleType::WritePortalType>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return SourceStorage::CreateBuffers();
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    SourceStorage::ResizeBuffers(numValues, buffers, preserve, token);
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(buffers);
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const T& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    vtkm::Id numValues = GetNumberOfValues(buffers);
    SourceStorage::Fill(buffers, fillValue, numValues - endIndex, numValues - startIndex, token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(SourceStorage::CreateReadPortal(buffers, device, token));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(SourceStorage::CreateWritePortal(buffers, device, token));
  }
}; // class storage

} // namespace internal

/// \brief Reverse the order of an array, on demand.
///
/// ArrayHandleReverse is a specialization of ArrayHandle. Given an ArrayHandle,
/// it creates a new handle that returns the elements of the array in reverse
/// order (i.e. from end to beginning).
///
template <typename ArrayHandleType>
class ArrayHandleReverse
  : public vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                   StorageTagReverse<typename ArrayHandleType::StorageTag>>

{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleReverse,
    (ArrayHandleReverse<ArrayHandleType>),
    (vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                             StorageTagReverse<typename ArrayHandleType::StorageTag>>));

public:
  ArrayHandleReverse(const ArrayHandleType& handle)
    : Superclass(handle.GetBuffers())
  {
  }

  VTKM_CONT ArrayHandleType GetSourceArray() const
  {
    return vtkm::cont::ArrayHandle<ValueType, typename ArrayHandleType::StorageTag>(
      this->GetBuffers());
  }
};

/// make_ArrayHandleReverse is convenience function to generate an
/// ArrayHandleReverse.
///
template <typename HandleType>
VTKM_CONT ArrayHandleReverse<HandleType> make_ArrayHandleReverse(const HandleType& handle)
{
  return ArrayHandleReverse<HandleType>(handle);
}

namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if
// the sub-storage is inefficient (thus making everything inefficient).
template <typename StorageTag>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagReverse<StorageTag>>
  : vtkm::cont::internal::ArrayExtractComponentImpl<StorageTag>
{
  template <typename T>
  using StrideArrayType =
    vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType>;

  template <typename T>
  StrideArrayType<T> operator()(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagReverse<StorageTag>>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy) const
  {
    vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<T, StorageTag>> srcArray(src);
    StrideArrayType<T> subArray =
      ArrayExtractComponentImpl<StorageTag>{}(srcArray.GetSourceArray(), componentIndex, allowCopy);
    // Reverse the array by starting at the end and striding backward
    return StrideArrayType<T>(subArray.GetBasicArray(),
                              srcArray.GetNumberOfValues(),
                              -subArray.GetStride(),
                              subArray.GetOffset() +
                                (subArray.GetStride() * (subArray.GetNumberOfValues() - 1)),
                              subArray.GetModulo(),
                              subArray.GetDivisor());
  }
};

} // namespace internal

}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename AH>
struct SerializableTypeString<vtkm::cont::ArrayHandleReverse<AH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Reverse<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST>
struct SerializableTypeString<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagReverse<ST>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<T, ST>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<vtkm::cont::ArrayHandleReverse<AH>>
{
private:
  using Type = vtkm::cont::ArrayHandleReverse<AH>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    vtkmdiy::save(bb, obj.GetSourceArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);
    obj = vtkm::cont::make_ArrayHandleReverse(array);
  }
};

template <typename T, typename ST>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagReverse<ST>>>
  : Serialization<vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<T, ST>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleReverse_h
