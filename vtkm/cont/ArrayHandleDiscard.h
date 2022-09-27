//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleDiscard_h
#define vtk_m_cont_ArrayHandleDiscard_h

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/internal/Unreachable.h>

#include <type_traits>

namespace vtkm
{
namespace exec
{
namespace internal
{

/// \brief An output-only array portal with no storage. All written values are
/// discarded.
template <typename ValueType_>
class ArrayPortalDiscard
{
public:
  using ValueType = ValueType_;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalDiscard()
    : NumberOfValues(0)
  {
  } // needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT
  explicit ArrayPortalDiscard(vtkm::Id numValues)
    : NumberOfValues(numValues)
  {
  }

  /// Copy constructor for any other ArrayPortalDiscard with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV>
  VTKM_CONT ArrayPortalDiscard(const ArrayPortalDiscard<OtherV>& src)
    : NumberOfValues(src.NumberOfValues)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  ValueType Get(vtkm::Id) const
  {
    VTKM_UNREACHABLE("Cannot read from ArrayHandleDiscard.");
    return vtkm::TypeTraits<ValueType>::ZeroInitialization();
  }

  VTKM_EXEC
  void Set(vtkm::Id index, const ValueType&) const
  {
    VTKM_ASSERT(index < this->GetNumberOfValues());
    (void)index;
    // no-op
  }

private:
  vtkm::Id NumberOfValues;
};

} // end namespace internal
} // end namespace exec

namespace cont
{

namespace internal
{

struct VTKM_ALWAYS_EXPORT StorageTagDiscard
{
};

struct VTKM_ALWAYS_EXPORT DiscardMetaData
{
  vtkm::Id NumberOfValues = 0;
};

template <typename ValueType>
class Storage<ValueType, StorageTagDiscard>
{
public:
  using WritePortalType = vtkm::exec::internal::ArrayPortalDiscard<ValueType>;

  // Note that this portal is write-only, so you will probably run into problems if
  // you actually try to use this read portal.
  using ReadPortalType = vtkm::exec::internal::ArrayPortalDiscard<ValueType>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    DiscardMetaData metaData;
    metaData.NumberOfValues = 0;
    return vtkm::cont::internal::CreateBuffers(metaData);
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag,
                                      vtkm::cont::Token&)
  {
    VTKM_ASSERT(numValues >= 0);
    buffers[0].GetMetaData<DiscardMetaData>().NumberOfValues = numValues;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<DiscardMetaData>().NumberOfValues;
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const ValueType&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    // Fill is a NO-OP.
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const std::vector<vtkm::cont::internal::Buffer>&,
                                                   vtkm::cont::DeviceAdapterId,
                                                   vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadValue("Cannot read from ArrayHandleDiscard.");
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId,
    vtkm::cont::Token&)
  {
    return WritePortalType(GetNumberOfValues(buffers));
  }
};

template <typename ValueType_>
struct ArrayHandleDiscardTraits
{
  using ValueType = ValueType_;
  using StorageTag = StorageTagDiscard;
  using Superclass = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

} // end namespace internal

/// ArrayHandleDiscard is a write-only array that discards all data written to
/// it. This can be used to save memory when a filter provides optional outputs
/// that are not needed.
template <typename ValueType_>
class ArrayHandleDiscard : public internal::ArrayHandleDiscardTraits<ValueType_>::Superclass
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleDiscard,
                             (ArrayHandleDiscard<ValueType_>),
                             (typename internal::ArrayHandleDiscardTraits<ValueType_>::Superclass));
};

/// Helper to determine if an ArrayHandle type is an ArrayHandleDiscard.
template <typename T>
struct IsArrayHandleDiscard : std::false_type
{
};

template <typename T>
struct IsArrayHandleDiscard<ArrayHandle<T, internal::StorageTagDiscard>> : std::true_type
{
};

} // end namespace cont
} // end namespace vtkm

#endif // vtk_m_cont_ArrayHandleDiscard_h
