//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleStride_h
#define vtk_m_cont_ArrayHandleStride_h

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/internal/ArrayPortalBasic.h>

namespace vtkm
{
namespace internal
{

struct ArrayStrideInfo
{
  vtkm::Id NumberOfValues = 0;
  vtkm::Id Stride = 1;
  vtkm::Id Offset = 0;
  vtkm::Id Modulo = 0;
  vtkm::Id Divisor = 0;

  ArrayStrideInfo() = default;

  ArrayStrideInfo(vtkm::Id numValues,
                  vtkm::Id stride,
                  vtkm::Id offset,
                  vtkm::Id modulo,
                  vtkm::Id divisor)
    : NumberOfValues(numValues)
    , Stride(stride)
    , Offset(offset)
    , Modulo(modulo)
    , Divisor(divisor)
  {
  }

  VTKM_EXEC_CONT vtkm::Id ArrayIndex(vtkm::Id index) const
  {
    vtkm::Id arrayIndex = index;
    if (this->Divisor > 1)
    {
      arrayIndex = arrayIndex / this->Divisor;
    }
    if (this->Modulo > 0)
    {
      arrayIndex = arrayIndex % this->Modulo;
    }
    arrayIndex = (arrayIndex * this->Stride) + this->Offset;
    return arrayIndex;
  }
};

template <typename T>
class ArrayPortalStrideRead
{
  const T* Array = nullptr;
  ArrayStrideInfo Info;

public:
  ArrayPortalStrideRead() = default;
  ArrayPortalStrideRead(ArrayPortalStrideRead&&) = default;
  ArrayPortalStrideRead(const ArrayPortalStrideRead&) = default;
  ArrayPortalStrideRead& operator=(ArrayPortalStrideRead&&) = default;
  ArrayPortalStrideRead& operator=(const ArrayPortalStrideRead&) = default;

  ArrayPortalStrideRead(const T* array, const ArrayStrideInfo& info)
    : Array(array)
    , Info(info)
  {
  }

  using ValueType = T;

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->Info.NumberOfValues; }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    return detail::ArrayPortalBasicReadGet(this->Array + this->Info.ArrayIndex(index));
  }

  VTKM_EXEC_CONT const ValueType* GetArray() const { return this->Array; }
  VTKM_EXEC_CONT const ArrayStrideInfo& GetInfo() const { return this->Info; }
};

template <typename T>
class ArrayPortalStrideWrite
{
  T* Array = nullptr;
  ArrayStrideInfo Info;

public:
  ArrayPortalStrideWrite() = default;
  ArrayPortalStrideWrite(ArrayPortalStrideWrite&&) = default;
  ArrayPortalStrideWrite(const ArrayPortalStrideWrite&) = default;
  ArrayPortalStrideWrite& operator=(ArrayPortalStrideWrite&&) = default;
  ArrayPortalStrideWrite& operator=(const ArrayPortalStrideWrite&) = default;

  ArrayPortalStrideWrite(T* array, const ArrayStrideInfo& info)
    : Array(array)
    , Info(info)
  {
  }

  using ValueType = T;

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->Info.NumberOfValues; }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    return detail::ArrayPortalBasicWriteGet(this->Array + this->Info.ArrayIndex(index));
  }

  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    detail::ArrayPortalBasicWriteSet(this->Array + this->Info.ArrayIndex(index), value);
  }

  VTKM_EXEC_CONT ValueType* GetArray() const { return this->Array; }
  VTKM_EXEC_CONT const ArrayStrideInfo& GetInfo() const { return this->Info; }
};

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagStride
{
};

namespace internal
{

template <typename T>
class VTKM_ALWAYS_EXPORT Storage<T, vtkm::cont::StorageTagStride>
{
  using StrideInfo = vtkm::internal::ArrayStrideInfo;

public:
  VTKM_STORAGE_NO_RESIZE;

  using ReadPortalType = vtkm::internal::ArrayPortalStrideRead<T>;
  using WritePortalType = vtkm::internal::ArrayPortalStrideWrite<T>;

  VTKM_CONT static StrideInfo& GetInfo(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<StrideInfo>();
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return GetInfo(buffers).NumberOfValues;
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const T&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadType("Fill not supported for ArrayHandleStride.");
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(reinterpret_cast<const T*>(buffers[1].ReadPointerDevice(device, token)),
                          GetInfo(buffers));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(reinterpret_cast<T*>(buffers[1].WritePointerDevice(device, token)),
                           GetInfo(buffers));
  }

  static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const vtkm::cont::internal::Buffer& sourceBuffer = vtkm::cont::internal::Buffer{},
    vtkm::internal::ArrayStrideInfo&& info = vtkm::internal::ArrayStrideInfo{})
  {
    return vtkm::cont::internal::CreateBuffers(info, sourceBuffer);
  }

  static vtkm::cont::ArrayHandleBasic<T> GetBasicArray(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>({ buffers[1] });
  }
};

} // namespace internal

/// \brief An `ArrayHandle` that accesses a basic array with strides and offsets.
///
/// `ArrayHandleStride` is a simple `ArrayHandle` that accesses data with a prescribed
/// stride and offset. You specify the stride and offset at construction. So when a portal
/// for this `ArrayHandle` `Get`s or `Set`s a value at a specific index, the value accessed
/// in the underlying C array is:
///
/// (index * stride) + offset
///
/// Optionally, you can also specify a modulo and divisor. If they are specified, the index
/// mangling becomes:
///
/// (((index / divisor) % modulo) * stride) + offset
///
/// You can "disable" any of the aforementioned operations by setting them to the following
/// values (most of which are arithmetic identities):
///
///   * stride: 1
///   * offset: 0
///   * modulo: 0
///   * divisor: 1
///
/// Note that all of these indices are referenced by the `ValueType` of the array. So, an
/// `ArrayHandleStride<vtkm::Float32>` with an offset of 1 will actually offset by 4 bytes
/// (the size of a `vtkm::Float32`).
///
/// `ArrayHandleStride` is used to provide a unified type for pulling a component out of
/// an `ArrayHandle`. This way, you can iterate over multiple components in an array without
/// having to implement a template instance for each vector size or representation.
///
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleStride
  : public vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagStride>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleStride,
                             (ArrayHandleStride<T>),
                             (ArrayHandle<T, vtkm::cont::StorageTagStride>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  ArrayHandleStride(vtkm::Id stride, vtkm::Id offset, vtkm::Id modulo = 0, vtkm::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        vtkm::cont::internal::Buffer{},
        vtkm::internal::ArrayStrideInfo(0, stride, offset, modulo, divisor)))
  {
  }

  ArrayHandleStride(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& array,
                    vtkm::Id numValues,
                    vtkm::Id stride,
                    vtkm::Id offset,
                    vtkm::Id modulo = 0,
                    vtkm::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        array.GetBuffers()[0],
        vtkm::internal::ArrayStrideInfo(numValues, stride, offset, modulo, divisor)))
  {
  }

  ArrayHandleStride(const vtkm::cont::internal::Buffer& buffer,
                    vtkm::Id numValues,
                    vtkm::Id stride,
                    vtkm::Id offset,
                    vtkm::Id modulo = 0,
                    vtkm::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        buffer,
        vtkm::internal::ArrayStrideInfo(numValues, stride, offset, modulo, divisor)))
  {
  }

  vtkm::Id GetStride() const { return StorageType::GetInfo(this->GetBuffers()).Stride; }
  vtkm::Id GetOffset() const { return StorageType::GetInfo(this->GetBuffers()).Offset; }
  vtkm::Id GetModulo() const { return StorageType::GetInfo(this->GetBuffers()).Modulo; }
  vtkm::Id GetDivisor() const { return StorageType::GetInfo(this->GetBuffers()).Divisor; }

  vtkm::cont::ArrayHandleBasic<T> GetBasicArray() const
  {
    return StorageType::GetBasicArray(this->GetBuffers());
  }
};

}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleStride<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AHStride<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagStride>>
  : SerializableTypeString<vtkm::cont::ArrayHandleStride<T>>
{
};

}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleStride<T>>
{
private:
  using BaseType = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagStride>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj_)
  {
    vtkm::cont::ArrayHandleStride<T> obj = obj_;
    vtkmdiy::save(bb, obj.GetNumberOfValues());
    vtkmdiy::save(bb, obj.GetStride());
    vtkmdiy::save(bb, obj.GetOffset());
    vtkmdiy::save(bb, obj.GetModulo());
    vtkmdiy::save(bb, obj.GetDivisor());
    vtkmdiy::save(bb, obj.GetBuffers()[1]);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id numValues;
    vtkm::Id stride;
    vtkm::Id offset;
    vtkm::Id modulo;
    vtkm::Id divisor;
    vtkm::cont::internal::Buffer buffer;

    vtkmdiy::load(bb, numValues);
    vtkmdiy::load(bb, stride);
    vtkmdiy::load(bb, offset);
    vtkmdiy::load(bb, modulo);
    vtkmdiy::load(bb, divisor);
    vtkmdiy::load(bb, buffer);

    obj = vtkm::cont::ArrayHandleStride<T>(buffer, stride, offset, modulo, divisor);
  }
};

} // namespace diy
/// @endcond SERIALIZATION

/// \cond
/// Make doxygen ignore this section
#ifndef vtk_m_cont_ArrayHandleStride_cxx

namespace vtkm
{
namespace cont
{

namespace internal
{

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<char, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int8, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt8, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int16, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt16, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int32, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt32, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int64, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt64, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Float32, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Float64, StorageTagStride>;

} // namespace internal

extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<char, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Int8, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::UInt8, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Int16, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::UInt16, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Int32, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::UInt32, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Int64, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::UInt64, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Float32, StorageTagStride>;
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Float64, StorageTagStride>;

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleStride_cxx
/// \endcond

#endif //vtk_m_cont_ArrayHandleStride_h
