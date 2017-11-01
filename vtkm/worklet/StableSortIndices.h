//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_SortAndUniqueIndices_h
#define vtk_m_worklet_SortAndUniqueIndices_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm
{
namespace worklet
{

/// Produces an ArrayHandle<vtkm::Id> index array that stable-sorts and
/// optionally uniquifies an input array.
template <typename DeviceAdapter>
struct StableSortIndices
{
  using IndexArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;

  // Allows Sort to be called on an array that indexes into KeyPortal.
  // If the values compare equal, the indices are compared to stabilize the
  // result.
  template <typename KeyPortalType>
  struct IndirectSortPredicate
  {
    using KeyType = typename KeyPortalType::ValueType;

    const KeyPortalType KeyPortal;

    VTKM_CONT
    IndirectSortPredicate(const KeyPortalType& keyPortal)
      : KeyPortal(keyPortal)
    {
    }

    template <typename IndexType>
    VTKM_EXEC bool operator()(const IndexType& a, const IndexType& b) const
    {
      // If the values compare equal, compare the indices as well so we get
      // consistent outputs.
      const KeyType valueA = this->KeyPortal.Get(a);
      const KeyType valueB = this->KeyPortal.Get(b);
      if (valueA < valueB)
      {
        return true;
      }
      else if (valueB < valueA)
      {
        return false;
      }
      else
      {
        return a < b;
      }
    }
  };

  // Allows Unique to be called on an array that indexes into KeyPortal.
  template <typename KeyPortalType>
  struct IndirectUniquePredicate
  {
    const KeyPortalType KeyPortal;

    VTKM_CONT
    IndirectUniquePredicate(const KeyPortalType& keyPortal)
      : KeyPortal(keyPortal)
    {
    }

    template <typename IndexType>
    VTKM_EXEC bool operator()(const IndexType& a, const IndexType& b) const
    {
      return this->KeyPortal.Get(a) == this->KeyPortal.Get(b);
    }
  };

  /// Permutes the @a indices array so that it will map @a keys into a stable
  /// sorted order. The @a keys array is not modified.
  ///
  /// @param keys The ArrayHandle containing data to be sorted.
  /// @param indices The ArrayHandle<vtkm::Id> containing the permutation indices.
  ///
  /// @note @a indices is expected to contain the values (0, numKeys] in
  /// increasing order. If the values in @a indices are not sequential, the sort
  /// will succeed and be consistently reproducible, but the result is not
  /// guaranteed to be stable with respect to the original ordering of @a keys.
  template <typename KeyType, typename Storage>
  VTKM_CONT static void Sort(const vtkm::cont::ArrayHandle<KeyType, Storage>& keys,
                             IndexArrayType& indices)
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using KeyArrayType = vtkm::cont::ArrayHandle<KeyType, Storage>;
    using KeyPortalType =
      typename KeyArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using SortPredicate = IndirectSortPredicate<KeyPortalType>;

    VTKM_ASSERT(keys.GetNumberOfValues() == indices.GetNumberOfValues());

    KeyPortalType keyPortal = keys.PrepareForInput(DeviceAdapter());
    Algo::Sort(indices, SortPredicate(keyPortal));
  }

  /// Returns an index array that maps the @a keys array into a stable sorted
  /// ordering. The @a keys array is not modified.
  ///
  /// This is a convenience overload that generates the index array.
  template <typename KeyType, typename Storage>
  VTKM_CONT static IndexArrayType Sort(const vtkm::cont::ArrayHandle<KeyType, Storage>& keys)
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using KeyArrayType = vtkm::cont::ArrayHandle<KeyType, Storage>;
    using KeyPortalType =
      typename KeyArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using SortPredicate = IndirectSortPredicate<KeyPortalType>;

    // Generate the initial index array
    IndexArrayType indices;
    {
      vtkm::cont::ArrayHandleIndex indicesSrc(keys.GetNumberOfValues());
      Algo::Copy(indicesSrc, indices);
    }

    KeyPortalType keyPortal = keys.PrepareForInput(DeviceAdapter());
    Algo::Sort(indices, SortPredicate(keyPortal));

    return indices;
  }

  /// Reduces the array returned by @a Sort so that the mapped @a keys are
  /// unique. The @a indices array will be modified in-place and the @a keys
  /// array is not modified.
  ///
  template <typename KeyType, typename Storage>
  VTKM_CONT static void Unique(const vtkm::cont::ArrayHandle<KeyType, Storage>& keys,
                               IndexArrayType& indices)
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using KeyArrayType = vtkm::cont::ArrayHandle<KeyType, Storage>;
    using KeyPortalType =
      typename KeyArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using UniquePredicate = IndirectUniquePredicate<KeyPortalType>;

    KeyPortalType keyPortal = keys.PrepareForInput(DeviceAdapter());
    Algo::Unique(indices, UniquePredicate(keyPortal));
  }
};
}
} // end namespace vtkm::worklet

#endif // vtk_m_worklet_SortAndUniqueIndices_h
