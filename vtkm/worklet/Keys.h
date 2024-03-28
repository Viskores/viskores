//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Keys_h
#define vtk_m_worklet_Keys_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/Deprecated.h>
#include <vtkm/Hash.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

#include <vtkm/cont/arg/TransportTagKeyedValuesIn.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesInOut.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesOut.h>
#include <vtkm/cont/arg/TransportTagKeysIn.h>
#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/worklet/internal/DispatcherBase.h>

#include <vtkm/worklet/StableSortIndices.h>
#include <vtkm/worklet/vtkm_worklet_export.h>

#include <vtkm/BinaryOperators.h>

namespace vtkm
{
namespace worklet
{

namespace internal
{

class VTKM_WORKLET_EXPORT KeysBase
{
public:
  KeysBase(const KeysBase&) = default;
  KeysBase& operator=(const KeysBase&) = default;
  ~KeysBase() = default;

  VTKM_CONT
  vtkm::Id GetInputRange() const { return this->Offsets.GetNumberOfValues() - 1; }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetSortedValuesMap() const { return this->SortedValuesMap; }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetOffsets() const { return this->Offsets; }

  VTKM_DEPRECATED(2.2, "Use the `GetOffsets()` array in an `ArrayHandleOffsetsToNumComponents`.")
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::IdComponent> GetCounts() const;

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->SortedValuesMap.GetNumberOfValues(); }

  using ExecLookup = vtkm::exec::internal::ReduceByKeyLookupBase<
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType,
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ReadPortalType>;

  VTKM_CONT ExecLookup PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                       vtkm::cont::Token& token) const
  {
    return ExecLookup(this->SortedValuesMap.PrepareForInput(device, token),
                      this->Offsets.PrepareForInput(device, token));
  }

  VTKM_CONT
  bool operator==(const vtkm::worklet::internal::KeysBase& other) const
  {
    return ((this->SortedValuesMap == other.SortedValuesMap) && (this->Offsets == other.Offsets) &&
            (this->Offsets == other.Offsets));
  }

  VTKM_CONT
  bool operator!=(const vtkm::worklet::internal::KeysBase& other) const
  {
    return !(*this == other);
  }

protected:
  KeysBase() = default;

  vtkm::cont::ArrayHandle<vtkm::Id> SortedValuesMap;
  vtkm::cont::ArrayHandle<vtkm::Id> Offsets;
};

} // namespace internal

/// Select the type of sort for BuildArrays calls. Unstable sorting is faster
/// but will not produce consistent ordering for equal keys. Stable sorting
/// is slower, but keeps equal keys in their original order.
enum class KeysSortType
{
  Unstable = 0,
  Stable = 1
};

/// \brief Manage keys for a `vtkm::worklet::WorkletReduceByKey`.
///
/// The `vtkm::worklet::WorkletReduceByKey` worklet takes an array of keys for
/// its input domain, finds all identical keys, and runs a worklet that produces
/// a single value for every key given all matching values. This class is used
/// as the associated input for the keys input domain.
///
/// `Keys` is templated on the key array handle type and accepts an instance
/// of this array handle as its constructor. It builds the internal structures
/// needed to use the keys.
///
/// The same `Keys` structure can be used for multiple different invokes of
/// different or the same worklets. When used in this way, the processing done in the
/// `Keys` structure is reused for all the invokes. This is more efficient than
/// creating a different `Keys` structure for each invoke.
///
template <typename T>
class VTKM_ALWAYS_EXPORT Keys : public internal::KeysBase
{
public:
  using KeyType = T;
  using KeyArrayHandleType = vtkm::cont::ArrayHandle<KeyType>;

  VTKM_CONT
  Keys();

  /// Construct a `Keys` class from an array of keys.
  ///
  /// Given an array of keys, construct a `Keys` class that will manage
  /// using these keys to perform reduce-by-key operations.
  ///
  /// The input keys object is not modified and the result is not stable
  /// sorted. This is the equivalent of calling
  /// `BuildArrays(keys, KeysSortType::Unstable, device)`.
  ///
  template <typename KeyStorage>
  VTKM_CONT Keys(const vtkm::cont::ArrayHandle<KeyType, KeyStorage>& keys,
                 vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
  {
    this->BuildArrays(keys, KeysSortType::Unstable, device);
  }

  /// Build the internal arrays without modifying the input. This is more
  /// efficient for stable sorted arrays, but requires an extra copy of the
  /// keys for unstable sorting.
  template <typename KeyArrayType>
  VTKM_CONT void BuildArrays(
    const KeyArrayType& keys,
    KeysSortType sort,
    vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

  /// Build the internal arrays and also sort the input keys. This is more
  /// efficient for unstable sorting, but requires an extra copy for stable
  /// sorting.
  template <typename KeyArrayType>
  VTKM_CONT void BuildArraysInPlace(
    KeyArrayType& keys,
    KeysSortType sort,
    vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny());

  /// Returns an array of unique keys. The order of keys in this array describes
  /// the order that result values will be placed in a `vtkm::worklet::WorkletReduceByKey`.
  VTKM_CONT
  KeyArrayHandleType GetUniqueKeys() const { return this->UniqueKeys; }

#ifdef VTKM_DOXYGEN_ONLY
  // Document the superclass' methods as methods in this class.

  /// @brief Returns the input range of a keys object when used as an input domain.
  ///
  /// This will be equal to the number of unique keys.
  vtkm::Id GetInputRange() const;

  /// @brief Returns the array that maps each input value to an array of sorted keys.
  ///
  /// This array is used internally as the indices to a `vtkm::cont::ArrayHandlePermutation`
  /// to order input values with the grouped keys so that they can then be grouped. This is
  /// an internal array that is seldom of use to code outside the
  /// `vtkm::worklet::WorkletReduceByKey` implementation.
  vtkm::cont::ArrayHandle<vtkm::Id> GetSortedValuesMap() const;

  /// @brief Returns an offsets array to group keys.
  ///
  /// Given an array of sorted keys (or more frequently values permuted to the sorting of
  /// the keys), this array of indices can be used as offsets for a
  /// `vtkm::cont::ArrayHandleGroupVecVariable`. This is an internal array that is seldom of
  /// use to code outside the `vtkm::worklet::WorkletReduceByKey` implementation.
  vtkm::cont::ArrayHandle<vtkm::Id> GetOffsets() const;

  /// @brief Returns the number of input keys and values used to build this structure.
  ///
  /// This is also the size of input arrays to a `vtkm::worklet::WorkletReduceByKey`.
  vtkm::Id GetNumberOfValues() const;
#endif

  using ExecLookup = vtkm::exec::internal::ReduceByKeyLookup<
    typename KeyArrayHandleType::ReadPortalType,
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType,
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ReadPortalType>;

  VTKM_CONT ExecLookup PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                       vtkm::cont::Token& token) const
  {
    return ExecLookup(this->UniqueKeys.PrepareForInput(device, token),
                      this->SortedValuesMap.PrepareForInput(device, token),
                      this->Offsets.PrepareForInput(device, token));
  }

  VTKM_CONT
  bool operator==(const vtkm::worklet::Keys<KeyType>& other) const
  {
    return ((this->UniqueKeys == other.UniqueKeys) &&
            (this->SortedValuesMap == other.SortedValuesMap) && (this->Offsets == other.Offsets));
  }

  VTKM_CONT
  bool operator!=(const vtkm::worklet::Keys<KeyType>& other) const { return !(*this == other); }

private:
  /// @cond NONE
  KeyArrayHandleType UniqueKeys;

  template <typename KeyArrayType>
  VTKM_CONT void BuildArraysInternal(KeyArrayType& keys, vtkm::cont::DeviceAdapterId device);

  template <typename KeyArrayType>
  VTKM_CONT void BuildArraysInternalStable(const KeyArrayType& keys,
                                           vtkm::cont::DeviceAdapterId device);
  /// @endcond
};

template <typename T>
VTKM_CONT Keys<T>::Keys() = default;

namespace internal
{

template <typename KeyType>
inline auto SchedulingRange(const vtkm::worklet::Keys<KeyType>& inputDomain)
  -> decltype(inputDomain.GetInputRange())
{
  return inputDomain.GetInputRange();
}

template <typename KeyType>
inline auto SchedulingRange(const vtkm::worklet::Keys<KeyType>* const inputDomain)
  -> decltype(inputDomain->GetInputRange())
{
  return inputDomain->GetInputRange();
}

inline auto SchedulingRange(const vtkm::worklet::internal::KeysBase& inputDomain)
  -> decltype(inputDomain.GetInputRange())
{
  return inputDomain.GetInputRange();
}

inline auto SchedulingRange(const vtkm::worklet::internal::KeysBase* const inputDomain)
  -> decltype(inputDomain->GetInputRange())
{
  return inputDomain->GetInputRange();
}
} // namespace internal
}
} // namespace vtkm::worklet

// Here we implement the type checks and transports that rely on the Keys
// class. We implement them here because the Keys class is not accessible to
// the arg classes. (The worklet package depends on the cont and exec packages,
// not the other way around.)

namespace vtkm
{
namespace cont
{
namespace arg
{

template <typename KeyType>
struct TypeCheck<vtkm::cont::arg::TypeCheckTagKeys, KeyType>
{
  static constexpr bool value =
    std::is_base_of<vtkm::worklet::internal::KeysBase, typename std::decay<KeyType>::type>::value;
};

template <typename KeyType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagKeysIn, KeyType, Device>
{
  using ContObjectType = KeyType;
  using ExecObjectType = typename ContObjectType::ExecLookup;

  VTKM_CONT
  ExecObjectType operator()(const ContObjectType& object,
                            const ContObjectType& inputDomain,
                            vtkm::Id,
                            vtkm::Id,
                            vtkm::cont::Token& token) const
  {
    if (object != inputDomain)
    {
      throw vtkm::cont::ErrorBadValue("A Keys object must be the input domain.");
    }

    return object.PrepareForInput(Device(), token);
  }

  // If you get a compile error here, it means that you have used a KeysIn
  // tag in your ControlSignature that was not marked as the InputDomain.
  template <typename InputDomainType>
  VTKM_CONT ExecObjectType
  operator()(const ContObjectType&, const InputDomainType&, vtkm::Id, vtkm::Id) const = delete;
};

template <typename ArrayHandleType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagKeyedValuesIn, ArrayHandleType, Device>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using PermutedArrayType = vtkm::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType = vtkm::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::ReadPortalType;

  VTKM_CONT ExecObjectType operator()(const ContObjectType& object,
                                      const vtkm::worklet::internal::KeysBase& keys,
                                      vtkm::Id,
                                      vtkm::Id,
                                      vtkm::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != keys.GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue("Input values array is wrong size.");
    }

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForInput(Device(), token);
  }
};

template <typename ArrayHandleType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagKeyedValuesInOut, ArrayHandleType, Device>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using PermutedArrayType = vtkm::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType = vtkm::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::WritePortalType;

  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const vtkm::worklet::internal::KeysBase& keys,
                                      vtkm::Id,
                                      vtkm::Id,
                                      vtkm::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != keys.GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue("Input/output values array is wrong size.");
    }

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForInPlace(Device(), token);
  }
};

template <typename ArrayHandleType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagKeyedValuesOut, ArrayHandleType, Device>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using PermutedArrayType = vtkm::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType = vtkm::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::WritePortalType;

  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const vtkm::worklet::internal::KeysBase& keys,
                                      vtkm::Id,
                                      vtkm::Id,
                                      vtkm::cont::Token& token) const
  {
    // The PrepareForOutput for ArrayHandleGroupVecVariable and
    // ArrayHandlePermutation cannot determine the actual size expected for the
    // target array (object), so we have to make sure it gets allocated here.
    object.PrepareForOutput(keys.GetNumberOfValues(), Device(), token);

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForOutput(keys.GetInputRange(), Device(), token);
  }
};
}
}
} // namespace vtkm::cont::arg

#ifndef vtk_m_worklet_Keys_cxx

#define VTK_M_KEYS_EXPORT(T)                                                                       \
  extern template class VTKM_WORKLET_TEMPLATE_EXPORT vtkm::worklet::Keys<T>;                       \
  extern template VTKM_WORKLET_TEMPLATE_EXPORT VTKM_CONT void vtkm::worklet::Keys<T>::BuildArrays( \
    const vtkm::cont::ArrayHandle<T>& keys,                                                        \
    vtkm::worklet::KeysSortType sort,                                                              \
    vtkm::cont::DeviceAdapterId device)

VTK_M_KEYS_EXPORT(vtkm::UInt8);
VTK_M_KEYS_EXPORT(vtkm::HashType);
VTK_M_KEYS_EXPORT(vtkm::Id);
VTK_M_KEYS_EXPORT(vtkm::Id2);
VTK_M_KEYS_EXPORT(vtkm::Id3);
using Pair_UInt8_Id2 = vtkm::Pair<vtkm::UInt8, vtkm::Id2>;
VTK_M_KEYS_EXPORT(Pair_UInt8_Id2);
#ifdef VTKM_USE_64BIT_IDS
VTK_M_KEYS_EXPORT(vtkm::IdComponent);
#endif

#undef VTK_M_KEYS_EXPORT

#endif // !vtk_m_worklet_Keys_cxx

#endif //vtk_m_worklet_Keys_h
