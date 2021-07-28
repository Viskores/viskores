//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleRecombineVec_h
#define vtk_m_cont_ArrayHandleRecombineVec_h

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/DeviceAdapterTag.h>

#include <vtkm/VecVariable.h>

#include <vtkm/internal/ArrayPortalValueReference.h>

namespace vtkm
{
namespace internal
{

template <typename PortalType>
class RecombineVec
{
  vtkm::VecCConst<PortalType> Portals;
  vtkm::Id Index;

public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  RecombineVec(const RecombineVec&) = default;

  VTKM_EXEC_CONT RecombineVec(const vtkm::VecCConst<PortalType>& portals, vtkm::Id index)
    : Portals(portals)
    , Index(index)
  {
  }

  VTKM_EXEC_CONT vtkm::IdComponent GetNumberOfComponents() const
  {
    return this->Portals.GetNumberOfComponents();
  }

  VTKM_EXEC_CONT
  vtkm::internal::ArrayPortalValueReference<PortalType> operator[](vtkm::IdComponent cIndex) const
  {
    return vtkm::internal::ArrayPortalValueReference<PortalType>(this->Portals[cIndex],
                                                                 this->Index);
  }

  template <typename T, vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT void CopyInto(vtkm::Vec<T, DestSize>& dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      dest[cIndex] = this->Portals[cIndex].Get(this->Index);
    }
    // Clear out any components not held by this dynamic Vec-like
    for (vtkm::IdComponent cIndex = numComponents; cIndex < DestSize; ++cIndex)
    {
      dest[cIndex] = vtkm::TypeTraits<T>::ZeroInitialization();
    }
  }

  VTKM_EXEC_CONT vtkm::Id GetIndex() const { return this->Index; }

  VTKM_EXEC_CONT RecombineVec& operator=(const RecombineVec& src)
  {
    this->DoCopy(src);
    return *this;
  }

  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator=(const T& src)
  {
    this->DoCopy(src);
    return *this;
  }

  VTKM_EXEC_CONT operator ComponentType() const { return this->Portals[0].Get(this->Index); }

  template <vtkm::IdComponent N>
  VTKM_EXEC_CONT operator vtkm::Vec<ComponentType, N>() const
  {
    vtkm::Vec<ComponentType, N> result;
    this->CopyInto(result);
    return result;
  }

  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator+=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] += VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator-=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] -= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator*=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] *= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator/=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] /= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator%=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] %= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator&=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] &= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator|=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] |= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator^=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] ^= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator>>=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] >>= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }
  template <typename T, typename = typename std::enable_if<vtkm::HasVecTraits<T>::value>::type>
  VTKM_EXEC_CONT RecombineVec& operator<<=(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    VTKM_ASSERT(this->GetNumberOfComponents() == VTraits::GetNumberOfComponents(src));
    for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
    {
      (*this)[cIndex] <<= VTraits::GetComponent(src, cIndex);
    }
    return *this;
  }

private:
  template <typename T>
  VTKM_EXEC_CONT void DoCopy(const T& src)
  {
    using VTraits = vtkm::VecTraits<T>;
    vtkm::IdComponent numComponents = VTraits::GetNumberOfComponents(src);
    if (numComponents > 1)
    {
      if (numComponents > this->GetNumberOfComponents())
      {
        numComponents = this->GetNumberOfComponents();
      }
      for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
      {
        this->Portals[cIndex].Set(this->Index,
                                  static_cast<ComponentType>(VTraits::GetComponent(src, cIndex)));
      }
    }
    else
    {
      // Special case when copying from a scalar
      for (vtkm::IdComponent cIndex = 0; cIndex < this->GetNumberOfComponents(); ++cIndex)
      {
        this->Portals[cIndex].Set(this->Index,
                                  static_cast<ComponentType>(VTraits::GetComponent(src, 0)));
      }
    }
  }
};

} // namespace internal

template <typename PortalType>
struct TypeTraits<vtkm::internal::RecombineVec<PortalType>>
{
private:
  using VecType = vtkm::internal::RecombineVec<PortalType>;
  using ComponentType = typename VecType::ComponentType;

public:
  using NumericTag = typename vtkm::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = vtkm::TypeTraitsVectorTag;

  VTKM_EXEC_CONT static vtkm::internal::RecombineVec<PortalType> ZeroInitialization()
  {
    // Return a vec-like of size 0.
    return vtkm::internal::RecombineVec<PortalType>{};
  }
};

template <typename PortalType>
struct VecTraits<vtkm::internal::RecombineVec<PortalType>>
{
  using VecType = vtkm::internal::RecombineVec<PortalType>;
  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename vtkm::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = vtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = vtkm::VecTraitsTagSizeVariable;

  VTKM_EXEC_CONT static vtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  VTKM_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VTKM_EXEC_CONT static void SetComponent(const VecType& vector,
                                          vtkm::IdComponent componentIndex,
                                          const ComponentType& component)
  {
    vector[componentIndex] = component;
  }

  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

namespace internal
{

template <typename SourcePortalType>
class ArrayPortalRecombineVec
{
  // Note that this ArrayPortal has a pointer to a C array of other portals. We need to
  // make sure that the pointer is valid on the device we are using it on. See the
  // CreateReadPortal and CreateWritePortal in the Storage below to see how that is
  // managed.
  const SourcePortalType* Portals;
  vtkm::IdComponent NumberOfComponents;

public:
  using ValueType = vtkm::internal::RecombineVec<SourcePortalType>;

  ArrayPortalRecombineVec() = default;
  ArrayPortalRecombineVec(const SourcePortalType* portals, vtkm::IdComponent numComponents)
    : Portals(portals)
    , NumberOfComponents(numComponents)
  {
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->Portals[0].GetNumberOfValues(); }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    return ValueType({ this->Portals, this->NumberOfComponents }, index);
  }

  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    // The ValueType is actually a reference back to the portals, and sets to it should
    // already be set in the portal. Thus, we don't really need to do anything.
    VTKM_ASSERT(value.GetIndex() == index);
  }

  template <typename T>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const T& value) const
  {
    using Traits = vtkm::VecTraits<T>;
    VTKM_ASSERT(Traits::GetNumberOfComponents(value) == this->NumberOfComponents);
    for (vtkm::IdComponent cIndex = 0; cIndex < this->NumberOfComponents; ++cIndex)
    {
      this->Portals[cIndex].Set(index, Traits::GetComponent(value, cIndex));
    }
  }
};

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

namespace internal
{

struct StorageTagRecombineVec
{
};

namespace detail
{

// Note: Normally a decorating ArrayHandle holds the buffers of the arrays it is decorating
// in its list of arrays. However, the numbers of buffers is expected to be compile-time static
// and ArrayHandleRecombineVec needs to set the number of buffers at runtime. We cheat around
// this by stuffing the decorated buffers in the metadata. To make sure deep copies work
// right, a copy of the metadata results in a deep copy of the contained buffers. The
// vtkm::cont::internal::Buffer holding the metadata is not supposed to copy the metadata
// except for a deep copy (and when it is first set). If this behavior changes, there could
// be a performance degredation.
struct RecombineVecMetaData
{
  mutable std::vector<vtkm::cont::internal::Buffer> PortalBuffers;
  std::vector<std::vector<vtkm::cont::internal::Buffer>> ArrayBuffers;

  RecombineVecMetaData() = default;

  RecombineVecMetaData(const RecombineVecMetaData& src) { *this = src; }

  RecombineVecMetaData& operator=(const RecombineVecMetaData& src)
  {
    this->ArrayBuffers.resize(src.ArrayBuffers.size());
    for (std::size_t arrayIndex = 0; arrayIndex < src.ArrayBuffers.size(); ++arrayIndex)
    {
      this->ArrayBuffers[arrayIndex].resize(src.ArrayBuffers[arrayIndex].size());
      for (std::size_t bufferIndex = 0; bufferIndex < src.ArrayBuffers[arrayIndex].size();
           ++bufferIndex)
      {
        this->ArrayBuffers[arrayIndex][bufferIndex].DeepCopyFrom(
          src.ArrayBuffers[arrayIndex][bufferIndex]);
      }
    }

    this->PortalBuffers.clear();
    // Intentionally not copying portals. Portals will be recreated from proper array when requsted.

    return *this;
  }
};

template <typename T>
using RecombinedPortalType = vtkm::internal::ArrayPortalMultiplexer<
  typename vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagStride>::ReadPortalType,
  typename vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagStride>::WritePortalType>;

template <typename T>
using RecombinedValueType = vtkm::internal::RecombineVec<RecombinedPortalType<T>>;

} // namespace detail

template <typename ReadWritePortal>
class Storage<vtkm::internal::RecombineVec<ReadWritePortal>,
              vtkm::cont::internal::StorageTagRecombineVec>
{
  using ComponentType = typename ReadWritePortal::ValueType;
  using SourceStorage = vtkm::cont::internal::Storage<ComponentType, vtkm::cont::StorageTagStride>;
  using ArrayType = vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagStride>;

  VTKM_STATIC_ASSERT(
    (std::is_same<ReadWritePortal, detail::RecombinedPortalType<ComponentType>>::value));

  template <typename Buff>
  VTKM_CONT static Buff* BuffersForComponent(Buff* buffers, vtkm::IdComponent componentIndex)
  {
    return buffers[0]
      .template GetMetaData<detail::RecombineVecMetaData>()
      .ArrayBuffers[componentIndex]
      .data();
  }

public:
  VTKM_STORAGE_NO_RESIZE;

  using ReadPortalType = vtkm::internal::ArrayPortalRecombineVec<ReadWritePortal>;
  using WritePortalType = vtkm::internal::ArrayPortalRecombineVec<ReadWritePortal>;

  VTKM_CONT static vtkm::IdComponent NumberOfComponents(const vtkm::cont::internal::Buffer* buffers)
  {
    return static_cast<vtkm::IdComponent>(
      buffers[0].GetMetaData<detail::RecombineVecMetaData>().ArrayBuffers.size());
  }

  VTKM_CONT static vtkm::IdComponent GetNumberOfBuffers() { return 1; }

  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return SourceStorage::GetNumberOfValues(BuffersForComponent(buffers, 0));
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token)
  {
    vtkm::IdComponent numComponents = NumberOfComponents(buffers);

    // The array portal needs a runtime-allocated array of portals for each component.
    // We use the vtkm::cont::internal::Buffer object to allow us to allocate memory on the
    // device and copy data there.
    vtkm::cont::internal::Buffer portalBuffer;
    portalBuffer.SetNumberOfBytes(static_cast<vtkm::BufferSizeType>(sizeof(ReadWritePortal)) *
                                    numComponents,
                                  vtkm::CopyFlag::Off,
                                  token);

    // Save a reference of the portal in our metadata.
    // Note that the buffer we create is going to hang around until the ArrayHandle gets
    // destroyed. The buffers are small and should not be a problem unless you create a
    // lot of portals.
    buffers[0].GetMetaData<detail::RecombineVecMetaData>().PortalBuffers.push_back(portalBuffer);

    // Get the control-side memory and fill it with the execution-side portals
    ReadWritePortal* portals =
      reinterpret_cast<ReadWritePortal*>(portalBuffer.WritePointerHost(token));
    for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      portals[cIndex] = ReadWritePortal(
        SourceStorage::CreateReadPortal(BuffersForComponent(buffers, cIndex), device, token));
    }

    // Now get the execution-side memory (portals will be copied as necessary) and create
    // the portal for the appropriate device
    return ReadPortalType(
      reinterpret_cast<const ReadWritePortal*>(portalBuffer.ReadPointerDevice(device, token)),
      numComponents);
  }

  VTKM_CONT static WritePortalType CreateWritePortal(vtkm::cont::internal::Buffer* buffers,
                                                     vtkm::cont::DeviceAdapterId device,
                                                     vtkm::cont::Token& token)
  {
    vtkm::IdComponent numComponents = NumberOfComponents(buffers);

    // The array portal needs a runtime-allocated array of portals for each component.
    // We use the vtkm::cont::internal::Buffer object to allow us to allocate memory on the
    // device and copy data there.
    vtkm::cont::internal::Buffer portalBuffer;
    portalBuffer.SetNumberOfBytes(static_cast<vtkm::BufferSizeType>(sizeof(ReadWritePortal)) *
                                    numComponents,
                                  vtkm::CopyFlag::Off,
                                  token);

    // Save a reference of the portal in our metadata.
    // Note that the buffer we create is going to hang around until the ArrayHandle gets
    // destroyed. The buffers are small and should not be a problem unless you create a
    // lot of portals.
    buffers[0].GetMetaData<detail::RecombineVecMetaData>().PortalBuffers.push_back(portalBuffer);

    // Get the control-side memory and fill it with the execution-side portals
    ReadWritePortal* portals =
      reinterpret_cast<ReadWritePortal*>(portalBuffer.WritePointerHost(token));
    for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      portals[cIndex] = ReadWritePortal(
        SourceStorage::CreateWritePortal(BuffersForComponent(buffers, cIndex), device, token));
    }

    // Now get the execution-side memory (portals will be copied as necessary) and create
    // the portal for the appropriate device
    return WritePortalType(
      reinterpret_cast<const ReadWritePortal*>(portalBuffer.ReadPointerDevice(device, token)),
      numComponents);
  }

  VTKM_CONT static ArrayType ArrayForComponent(const vtkm::cont::internal::Buffer* buffers,
                                               vtkm::IdComponent componentIndex)
  {
    return ArrayType(BuffersForComponent(buffers, componentIndex));
  }

  VTKM_CONT static void AppendComponent(vtkm::cont::internal::Buffer* buffers,
                                        const ArrayType& array)
  {
    std::vector<vtkm::cont::internal::Buffer> arrayBuffers(
      array.GetBuffers(), array.GetBuffers() + SourceStorage::GetNumberOfBuffers());
    buffers[0].GetMetaData<detail::RecombineVecMetaData>().ArrayBuffers.push_back(
      std::move(arrayBuffers));
  }
};

} // namespace internal

/// \brief A grouping of `ArrayHandleStride`s into an `ArrayHandle` of `Vec`s.
///
/// The main intention of `ArrayHandleStride` is to pull out a component of an
/// `ArrayHandle` without knowing there `ArrayHandle`'s storage or `Vec` shape.
/// However, usually you want to do an operation on all the components together.
/// `ArrayHandleRecombineVec` implements the functionality to easily take a
/// group of extracted components and treat them as a single `ArrayHandle` of
/// `Vec` values.
///
/// Note that caution should be used with `ArrayHandleRecombineVec` because the
/// size of the `Vec` values is not known at compile time. Thus, the value
/// type of this array is forced to a `VecVariable`, which can cause surprises
/// if treated as a `Vec`. In particular, the static `NUM_COMPONENTS` expression
/// does not exist.
///
template <typename ComponentType>
class ArrayHandleRecombineVec
  : public vtkm::cont::ArrayHandle<internal::detail::RecombinedValueType<ComponentType>,
                                   vtkm::cont::internal::StorageTagRecombineVec>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRecombineVec,
    (ArrayHandleRecombineVec<ComponentType>),
    (vtkm::cont::ArrayHandle<internal::detail::RecombinedValueType<ComponentType>,
                             vtkm::cont::internal::StorageTagRecombineVec>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  vtkm::IdComponent GetNumberOfComponents() const
  {
    return StorageType::NumberOfComponents(this->GetBuffers());
  }

  vtkm::cont::ArrayHandleStride<ComponentType> GetComponentArray(
    vtkm::IdComponent componentIndex) const
  {
    return StorageType::ArrayForComponent(this->GetBuffers(), componentIndex);
  }

  void AppendComponentArray(
    const vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagStride>& array)
  {
    StorageType::AppendComponent(this->GetBuffers(), array);
  }
};

namespace internal
{

template <>
struct ArrayExtractComponentImpl<vtkm::cont::internal::StorageTagRecombineVec>
{
  template <typename RecombineVec>
  vtkm::cont::ArrayHandleStride<
    typename vtkm::VecFlat<typename RecombineVec::ComponentType>::ComponentType>
  operator()(
    const vtkm::cont::ArrayHandle<RecombineVec, vtkm::cont::internal::StorageTagRecombineVec>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy) const
  {
    using ComponentType = typename RecombineVec::ComponentType;
    vtkm::cont::ArrayHandleRecombineVec<ComponentType> array(src);
    constexpr vtkm::IdComponent subComponents = vtkm::VecFlat<ComponentType>::NUM_COMPONENTS;
    return vtkm::cont::ArrayExtractComponent(
      array.GetComponentArray(componentIndex / subComponents),
      componentIndex % subComponents,
      allowCopy);
  }
};

} // namespace internal

}
} // namespace vtkm::cont

//=============================================================================
// Specializations of worklet arguments using ArrayHandleGropuVecVariable
#include <vtkm/exec/arg/FetchTagArrayDirectOutArrayHandleRecombineVec.h>

#endif //vtk_m_cont_ArrayHandleRecombineVec_h
