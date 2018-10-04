//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_Keys_h
#define vtk_m_worklet_Keys_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

#include <vtkm/cont/arg/TransportTagKeyedValuesIn.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesInOut.h>
#include <vtkm/cont/arg/TransportTagKeyedValuesOut.h>
#include <vtkm/cont/arg/TransportTagKeysIn.h>
#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/worklet/StableSortIndices.h>

#include <vtkm/BinaryOperators.h>

namespace vtkm
{
namespace worklet
{

/// \brief Manage keys for a \c WorkletReduceByKey.
///
/// The \c WorkletReduceByKey worklet (and its associated \c
/// DispatcherReduceByKey) take an array of keys for its input domain, find all
/// identical keys, and runs a worklet that produces a single value for every
/// key given all matching values. This class is used as the associated input
/// for the keys input domain.
///
/// \c Keys is templated on the key array handle type and accepts an instance
/// of this array handle as its constructor. It builds the internal structures
/// needed to use the keys.
///
/// The same \c Keys structure can be used for multiple different \c Invoke of
/// different dispatchers. When used in this way, the processing done in the \c
/// Keys structure is reused for all the \c Invoke. This is more efficient than
/// creating a different \c Keys structure for each \c Invoke.
///
template <typename _KeyType>
class Keys
{
public:
  using KeyType = _KeyType;
  using KeyArrayHandleType = vtkm::cont::ArrayHandle<KeyType>;

  /// Select the type of sort for BuildArrays calls. Unstable sorting is faster
  /// but will not produce consistent ordering for equal keys. Stable sorting
  /// is slower, but keeps equal keys in their original order.
  enum class SortType
  {
    Unstable = 0,
    Stable = 1
  };

  VTKM_CONT
  Keys() = default;

  /// \b Construct a Keys class from an array of keys.
  ///
  /// Given an array of keys, construct a \c Keys class that will manage
  /// using these keys to perform reduce-by-key operations.
  ///
  /// The input keys object is not modified and the result is not stable
  /// sorted. This is the equivalent of calling
  /// `BuildArrays(keys, SortType::Unstable, device)`.
  ///
  template <typename KeyStorage, typename Device>
  VTKM_CONT Keys(const vtkm::cont::ArrayHandle<KeyType, KeyStorage>& keys, Device)
  {
    this->BuildArrays(keys, SortType::Unstable, Device());
  }

  /// Build the internal arrays without modifying the input. This is more
  /// efficient for stable sorted arrays, but requires an extra copy of the
  /// keys for unstable sorting.
  template <typename KeyArrayType, typename Device>
  VTKM_CONT void BuildArrays(const KeyArrayType& keys, SortType sort, Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArrays");

    switch (sort)
    {
      case SortType::Unstable:
      {
        KeyArrayHandleType mutableKeys;
        Algorithm::Copy(keys, mutableKeys);

        this->BuildArraysInternal(mutableKeys, Device());
      }
      break;
      case SortType::Stable:
        this->BuildArraysInternalStable(keys, Device());
        break;
    }
  }

  /// Build the internal arrays and also sort the input keys. This is more
  /// efficient for unstable sorting, but requires an extra copy for stable
  /// sorting.
  template <typename KeyArrayType, typename Device>
  VTKM_CONT void BuildArraysInPlace(KeyArrayType& keys, SortType sort, Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArraysInPlace");

    switch (sort)
    {
      case SortType::Unstable:
        this->BuildArraysInternal(keys, Device());
        break;
      case SortType::Stable:
      {
        this->BuildArraysInternalStable(keys, Device());
        KeyArrayHandleType tmp;
        // Copy into a temporary array so that the permutation array copy
        // won't alias input/output memory:
        Algorithm::Copy(keys, tmp);
        Algorithm::Copy(vtkm::cont::make_ArrayHandlePermutation(this->SortedValuesMap, tmp), keys);
      }
      break;
    }
  }

  VTKM_CONT
  vtkm::Id GetInputRange() const { return this->UniqueKeys.GetNumberOfValues(); }

  VTKM_CONT
  KeyArrayHandleType GetUniqueKeys() const { return this->UniqueKeys; }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetSortedValuesMap() const { return this->SortedValuesMap; }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetOffsets() const { return this->Offsets; }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::IdComponent> GetCounts() const { return this->Counts; }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->SortedValuesMap.GetNumberOfValues(); }

  template <typename Device>
  struct ExecutionTypes
  {
    using KeyPortal = typename KeyArrayHandleType::template ExecutionTypes<Device>::PortalConst;
    using IdPortal =
      typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::PortalConst;
    using IdComponentPortal = typename vtkm::cont::ArrayHandle<
      vtkm::IdComponent>::template ExecutionTypes<Device>::PortalConst;

    using Lookup = vtkm::exec::internal::ReduceByKeyLookup<KeyPortal, IdPortal, IdComponentPortal>;
  };

  template <typename Device>
  VTKM_CONT typename ExecutionTypes<Device>::Lookup PrepareForInput(Device) const
  {
    return typename ExecutionTypes<Device>::Lookup(this->UniqueKeys.PrepareForInput(Device()),
                                                   this->SortedValuesMap.PrepareForInput(Device()),
                                                   this->Offsets.PrepareForInput(Device()),
                                                   this->Counts.PrepareForInput(Device()));
  }

  VTKM_CONT
  bool operator==(const vtkm::worklet::Keys<KeyType>& other) const
  {
    return ((this->UniqueKeys == other.UniqueKeys) &&
            (this->SortedValuesMap == other.SortedValuesMap) && (this->Offsets == other.Offsets) &&
            (this->Counts == other.Counts));
  }

  VTKM_CONT
  bool operator!=(const vtkm::worklet::Keys<KeyType>& other) const { return !(*this == other); }

private:
  KeyArrayHandleType UniqueKeys;
  vtkm::cont::ArrayHandle<vtkm::Id> SortedValuesMap;
  vtkm::cont::ArrayHandle<vtkm::Id> Offsets;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> Counts;

  template <typename KeyArrayType, typename Device>
  VTKM_CONT void BuildArraysInternal(KeyArrayType& keys, Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArraysInternal");

    const vtkm::Id numKeys = keys.GetNumberOfValues();

    Algorithm::Copy(vtkm::cont::ArrayHandleIndex(numKeys), this->SortedValuesMap);

    // TODO: Do we need the ability to specify a comparison functor for sort?
    Algorithm::SortByKey(keys, this->SortedValuesMap);

    // Find the unique keys and the number of values per key.
    Algorithm::ReduceByKey(keys,
                           vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(1, numKeys),
                           this->UniqueKeys,
                           this->Counts,
                           vtkm::Sum());

    // Get the offsets from the counts with a scan.
    vtkm::Id offsetsTotal = Algorithm::ScanExclusive(
      vtkm::cont::make_ArrayHandleCast(this->Counts, vtkm::Id()), this->Offsets);
    VTKM_ASSERT(offsetsTotal == numKeys); // Sanity check
    (void)offsetsTotal;                   // Shut up, compiler
  }

  template <typename KeyArrayType, typename Device>
  VTKM_CONT void BuildArraysInternalStable(const KeyArrayType& keys, Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Keys::BuildArraysInternalStable");

    const vtkm::Id numKeys = keys.GetNumberOfValues();

    // Produce a stable sorted map of the keys:
    this->SortedValuesMap = StableSortIndices<Device>::Sort(keys);
    auto sortedKeys = vtkm::cont::make_ArrayHandlePermutation(this->SortedValuesMap, keys);

    // Find the unique keys and the number of values per key.
    Algorithm::ReduceByKey(sortedKeys,
                           vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(1, numKeys),
                           this->UniqueKeys,
                           this->Counts,
                           vtkm::Sum());

    // Get the offsets from the counts with a scan.
    vtkm::Id offsetsTotal = Algorithm::ScanExclusive(
      vtkm::cont::make_ArrayHandleCast(this->Counts, vtkm::Id()), this->Offsets);
    VTKM_ASSERT(offsetsTotal == numKeys); // Sanity check
    (void)offsetsTotal;                   // Shut up, compiler
  }
};
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
struct TypeCheck<vtkm::cont::arg::TypeCheckTagKeys, vtkm::worklet::Keys<KeyType>>
{
  static constexpr bool value = true;
};

template <typename KeyType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagKeysIn, vtkm::worklet::Keys<KeyType>, Device>
{
  using ContObjectType = vtkm::worklet::Keys<KeyType>;
  using ExecObjectType = typename ContObjectType::template ExecutionTypes<Device>::Lookup;

  VTKM_CONT
  ExecObjectType operator()(const ContObjectType& object,
                            const ContObjectType& inputDomain,
                            vtkm::Id,
                            vtkm::Id) const
  {
    if (object != inputDomain)
    {
      throw vtkm::cont::ErrorBadValue("A Keys object must be the input domain.");
    }

    return object.PrepareForInput(Device());
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

  using ExecObjectType = typename GroupedArrayType::template ExecutionTypes<Device>::PortalConst;

  template <typename KeyType>
  VTKM_CONT ExecObjectType operator()(const ContObjectType& object,
                                      const vtkm::worklet::Keys<KeyType>& keys,
                                      vtkm::Id,
                                      vtkm::Id) const
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
    return groupedArray.PrepareForInput(Device());
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

  using ExecObjectType = typename GroupedArrayType::template ExecutionTypes<Device>::Portal;

  template <typename KeyType>
  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const vtkm::worklet::Keys<KeyType>& keys,
                                      vtkm::Id,
                                      vtkm::Id) const
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
    return groupedArray.PrepareForInPlace(Device());
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

  using ExecObjectType = typename GroupedArrayType::template ExecutionTypes<Device>::Portal;

  template <typename KeyType>
  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const vtkm::worklet::Keys<KeyType>& keys,
                                      vtkm::Id,
                                      vtkm::Id) const
  {
    // The PrepareForOutput for ArrayHandleGroupVecVariable and
    // ArrayHandlePermutation cannot determine the actual size expected for the
    // target array (object), so we have to make sure it gets allocated here.
    object.PrepareForOutput(keys.GetNumberOfValues(), Device());

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForOutput(keys.GetInputRange(), Device());
  }
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_worklet_Keys_h
