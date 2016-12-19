//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

#include <vtkm/cont/arg/TransportTagKeysIn.h>
#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/BinaryOperators.h>

namespace vtkm {
namespace worklet {

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
template<typename _KeyType>
class Keys
{
public:
  using KeyType = _KeyType;
  using KeyArrayHandleType = vtkm::cont::ArrayHandle<KeyType>;

  VTKM_CONT
  Keys()
  {  }

  template<typename OriginalKeyStorage, typename Device>
  VTKM_CONT
  Keys(const vtkm::cont::ArrayHandle<KeyType,OriginalKeyStorage> &keys,
       Device)
  {
    this->BuildArrays(keys, Device());
  }

  VTKM_CONT
  vtkm::Id GetInputRange() const
  {
    return this->UniqueKeys.GetNumberOfValues();
  }

  VTKM_CONT
  KeyArrayHandleType GetUniqueKeys() const
  {
    return this->UniqueKeys;
  }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetSortedValuesMap() const
  {
    return this->SortedValuesMap;
  }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetOffsets() const
  {
    return this->Offsets;
  }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::IdComponent> GetCounts() const
  {
    return this->Counts;
  }

  template<typename Device>
  struct ExecutionTypes
  {
    using KeyPortal = typename KeyArrayHandleType::template ExecutionTypes<Device>::PortalConst;
    using IdPortal = typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::PortalConst;
    using IdComponentPortal = typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::template ExecutionTypes<Device>::PortalConst;

    using Lookup = vtkm::exec::internal::ReduceByKeyLookup<KeyPortal,IdPortal,IdComponentPortal>;
  };

  template<typename Device>
  VTKM_CONT
  typename ExecutionTypes<Device>::Lookup PrepareForInput(Device) const
  {
    return typename ExecutionTypes<Device>::Lookup(
          this->UniqueKeys.PrepareForInput(Device()),
          this->SortedValuesMap.PrepareForInput(Device()),
          this->Offsets.PrepareForInput(Device()),
          this->Counts.PrepareForInput(Device()));
  }

  VTKM_CONT
  bool operator==(const vtkm::worklet::Keys<KeyType> &other) const
  {
    return ((this->UniqueKeys == other.UniqueKeys) &&
            (this->SortedValuesMap == other.SortedValuesMap) &&
            (this->Offsets == other.Offsets) &&
            (this->Counts == other.Counts));
  }

private:
  KeyArrayHandleType UniqueKeys;
  vtkm::cont::ArrayHandle<vtkm::Id> SortedValuesMap;
  vtkm::cont::ArrayHandle<vtkm::Id> Offsets;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> Counts;

  template<typename OriginalKeyArrayType, typename Device>
  VTKM_CONT
  void BuildArrays(const OriginalKeyArrayType &originalKeys, Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    vtkm::Id numOriginalKeys = originalKeys.GetNumberOfValues();

    // Copy and sort the keys. (Copy is in place.)
    KeyArrayHandleType sortedKeys;
    Algorithm::Copy(originalKeys, sortedKeys);

    Algorithm::Copy(vtkm::cont::ArrayHandleIndex(numOriginalKeys),
                    this->SortedValuesMap);

    // TODO: Do we need the ability to specify a comparison functor for sort?
    Algorithm::SortByKey(sortedKeys, this->SortedValuesMap);

    // Find the unique keys and the number of values per key.
    Algorithm::ReduceByKey(
          sortedKeys,
          vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(1, numOriginalKeys),
          this->UniqueKeys,
          this->Counts,
          vtkm::Sum());

    // Get the offsets from the counts with a scan.
    vtkm::Id offsetsTotal =
        Algorithm::ScanExclusive(
          vtkm::cont::make_ArrayHandleCast(this->Counts, vtkm::Id()),
          this->Offsets);
    VTKM_ASSERT(offsetsTotal == numOriginalKeys); // Sanity check
    (void)offsetsTotal; // Shut up, compiler
  }
};

}
} // namespace vtkm::worklet

// Here we implement the type checks, transports, and fetches the rely on the
// Keys class. We implement them here because the Keys class is not accessible
// to the arg classes. (The worklet package depends on the cont and exec
// packages, not the other way around.)

namespace vtkm {
namespace cont {
namespace arg {

template<typename KeyType>
struct TypeCheck<vtkm::cont::arg::TypeCheckTagKeys,
                 vtkm::worklet::Keys<KeyType> >
{
  static const bool value = true;
};

template<typename KeyType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagKeysIn,
                 vtkm::worklet::Keys<KeyType>,
                 Device>
{
  using ContObjectType = vtkm::worklet::Keys<KeyType>;
  using ExecObjectType =
      typename ContObjectType::template ExecutionTypes<Device>::Lookup;

  VTKM_CONT
  ExecObjectType operator()(const ContObjectType &object,
                            const ContObjectType &inputDomain,
                            vtkm::Id) const
  {
    VTKM_ASSERT(object == inputDomain);

    return object.PrepareForInput(Device());
  }

  template<typename InputDomainType>
  VTKM_CONT
  ExecObjectType operator()(const ContObjectType &,
                            const InputDomainType &,
                            vtkm::Id) const
  {
    // If you get a compile error here, it means that you have used a KeysIn
    // tag in your ControlSignature that was not marked as the InputDomain.
    VTKM_STATIC_ASSERT_MSG(
          false, "A Keys class was used in a position that is not the input domain.");
    return ExecObjectType();
  }
};

}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_worklet_Keys_h
