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
#include <vtkm/cont/internal/ArrayCopyUnknown.h>

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
  using ReadPortalType = vtkm::internal::ArrayPortalStrideRead<T>;
  using WritePortalType = vtkm::internal::ArrayPortalStrideWrite<T>;

  VTKM_CONT static StrideInfo& GetInfo(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<StrideInfo>();
  }

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>&)
  {
    return vtkm::VecFlat<T>::NUM_COMPONENTS;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return GetInfo(buffers).NumberOfValues;
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    StrideInfo& info = GetInfo(buffers);

    if (info.NumberOfValues == numValues)
    {
      // Array resized to current size. Don't need to do anything.
      return;
    }

    // Find the end index after dealing with the divsor and modulo.
    auto lengthDivMod = [info](vtkm::Id length) -> vtkm::Id {
      vtkm::Id resultLength = ((length - 1) / info.Divisor) + 1;
      if ((info.Modulo > 0) && (info.Modulo < resultLength))
      {
        resultLength = info.Modulo;
      }
      return resultLength;
    };
    vtkm::Id lastStridedIndex = lengthDivMod(numValues);

    vtkm::Id originalStride;
    vtkm::Id originalOffset;
    if (info.Stride > 0)
    {
      originalStride = info.Stride;
      originalOffset = info.Offset;
    }
    else
    {
      // The stride is negative, which means we are counting backward. Here we have to be careful
      // about the offset, which should move to push to the end of the array. We also need to
      // be careful about multiplying by the stride.
      originalStride = -info.Stride;

      vtkm::Id originalSize = lengthDivMod(info.NumberOfValues);

      // Because the stride is negative, we expect the offset to be at the end of the array.
      // We will call the "real" offset the distance from that end.
      originalOffset = originalSize - info.Offset - 1;
    }

    // If the offset is more than the stride, that means there are values skipped at the
    // beginning of the array, and it is impossible to know exactly how many. In this case,
    // we cannot know how to resize. (If this is an issue, we will have to change
    // `ArrayHandleStride` to take resizing parameters.)
    if (originalOffset >= originalStride)
    {
      if (numValues == 0)
      {
        // Array resized to zero. This can happen when releasing resources.
        // Should we try to clear out the buffers, or avoid that for messing up shared buffers?
        return;
      }
      throw vtkm::cont::ErrorBadAllocation(
        "Cannot resize stride array with offset greater than stride (start of stride unknown).");
    }

    // lastIndex should be the index in the source array after each stride block. Assuming the
    // offset is inside the first stride, this should be the end of the array regardless of
    // offset.
    vtkm::Id lastIndex = lastStridedIndex * originalStride;

    buffers[1].SetNumberOfBytes(
      vtkm::internal::NumberOfValuesToNumberOfBytes<T>(lastIndex), preserve, token);
    info.NumberOfValues = numValues;

    if (info.Stride < 0)
    {
      // As described above, when the stride is negative, we are counting backward. This means
      // that the offset is actually relative to the end, so we need to adjust it to the new
      // end of the array.
      info.Offset = lastIndex - originalOffset - 1;
    }
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const T& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token);

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

  ArrayHandleStride(vtkm::Id stride, vtkm::Id offset, vtkm::Id modulo = 0, vtkm::Id divisor = 1)
    : Superclass(StorageType::CreateBuffers(
        vtkm::cont::internal::Buffer{},
        vtkm::internal::ArrayStrideInfo(0, stride, offset, modulo, divisor)))
  {
  }

  /// @brief Construct an `ArrayHandleStride` from a basic array with specified access patterns.
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

  /// @brief Get the stride that values are accessed.
  ///
  /// The stride is the spacing between consecutive values. The stride is measured
  /// in terms of the number of values. A stride of 1 means a fully packed array.
  /// A stride of 2 means selecting every other values.
  vtkm::Id GetStride() const { return StorageType::GetInfo(this->GetBuffers()).Stride; }

  /// @brief Get the offset to start reading values.
  ///
  /// The offset is the number of values to skip before the first value. The offset
  /// is measured in terms of the number of values. An offset of 0 means the first value
  /// at the beginning of the array.
  ///
  /// The offset is unaffected by the stride and dictates where the strides starts
  /// counting. For example, given an array with size 3 vectors packed into an array,
  /// a strided array referencing the middle component will have offset 1 and stride 3.
  vtkm::Id GetOffset() const { return StorageType::GetInfo(this->GetBuffers()).Offset; }

  /// @brief Get the modulus of the array index.
  ///
  /// When the index is modulo a value, it becomes the remainder after dividing by that
  /// value. The effect of the modulus is to cause the index to repeat over the values
  /// in the array.
  ///
  /// If the modulo is set to 0, then it is ignored.
  vtkm::Id GetModulo() const { return StorageType::GetInfo(this->GetBuffers()).Modulo; }

  /// @brief Get the divisor of the array index.
  ///
  /// The index is divided by the divisor before the other effects. The default divisor of
  /// 1 will have no effect on the indexing. Setting the divisor to a value greater than 1
  /// has the effect of repeating each value that many times.
  vtkm::Id GetDivisor() const { return StorageType::GetInfo(this->GetBuffers()).Divisor; }

  /// @brief Return the underlying data as a basic array handle.
  ///
  /// It is common for the same basic array to be shared among multiple
  /// `vtkm::cont::ArrayHandleStride` objects.
  vtkm::cont::ArrayHandleBasic<T> GetBasicArray() const
  {
    return StorageType::GetBasicArray(this->GetBuffers());
  }
};

/// @brief Create an array by adding a stride to a basic array.
///
template <typename T>
vtkm::cont::ArrayHandleStride<T> make_ArrayHandleStride(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& array,
  vtkm::Id numValues,
  vtkm::Id stride,
  vtkm::Id offset,
  vtkm::Id modulo = 0,
  vtkm::Id divisor = 1)
{
  return { array, numValues, stride, offset, modulo, divisor };
}

}
} // namespace vtkm::cont

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T>
VTKM_CONT inline void Storage<T, vtkm::cont::StorageTagStride>::Fill(
  const std::vector<vtkm::cont::internal::Buffer>& buffers,
  const T& fillValue,
  vtkm::Id startIndex,
  vtkm::Id endIndex,
  vtkm::cont::Token& token)
{
  const StrideInfo& info = GetInfo(buffers);
  vtkm::cont::ArrayHandleBasic<T> basicArray = GetBasicArray(buffers);
  if ((info.Stride == 1) && (info.Modulo == 0) && (info.Divisor <= 1))
  {
    // Standard stride in array allows directly calling fill on the basic array.
    basicArray.Fill(fillValue, startIndex + info.Offset, endIndex + info.Offset, token);
  }
  else
  {
    // The fill does not necessarily cover a contiguous region. We have to have a loop
    // to set it. But we are not allowed to write device code here. Instead, create
    // a stride array containing the fill value with a modulo of 1 so that this fill
    // value repeates. Then feed this into a precompiled array copy that supports
    // stride arrays.
    const vtkm::Id numFill = endIndex - startIndex;
    auto fillValueArray = vtkm::cont::make_ArrayHandle({ fillValue });
    vtkm::cont::ArrayHandleStride<T> constantArray(fillValueArray, numFill, 1, 0, 1, 1);
    vtkm::cont::ArrayHandleStride<T> outputView(GetBasicArray(buffers),
                                                numFill,
                                                info.Stride,
                                                info.ArrayIndex(startIndex),
                                                info.Modulo,
                                                info.Divisor);
    // To prevent circular dependencies, this header file does not actually include
    // UnknownArrayHandle.h. Thus, it is possible to get a compile error on the following
    // line for using a declared but not defined `UnknownArrayHandle`. In the unlikely
    // event this occurs, simply include `vtkm/cont/UnknownArrayHandle.h` somewhere up the
    // include chain.
    vtkm::cont::internal::ArrayCopyUnknown(constantArray, outputView);
  }
}

}
}
} // namespace vtkm::cont::internal

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
