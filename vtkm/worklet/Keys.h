//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
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
template<typename _ValueType>
class Keys
{
public:
  using ValueType = _ValueType;
  using KeyArrayHandleType = vtkm::cont::ArrayHandle<ValueType>;

  template<typename OriginalKeyStorage, typename Device>
  VTKM_CONT
  Keys(const vtkm::cont::ArrayHandle<ValueType,OriginalKeyStorage> &keys,
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

#endif //vtk_m_worklet_Keys_h
