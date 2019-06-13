//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_internal_ReduceByKeyLookup_h
#define vtk_m_exec_internal_ReduceByKeyLookup_h

#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>

#include <type_traits>

namespace vtkm
{
namespace exec
{
namespace internal
{

/// \brief Execution object holding lookup info for reduce by key.
///
/// A WorkletReduceByKey needs several arrays to map the current output object
/// to the respective key and group of values. This execution object holds that
/// state.
///
template <typename KeyPortalType, typename IdPortalType, typename IdComponentPortalType>
struct ReduceByKeyLookup : vtkm::cont::ExecutionObjectBase
{
  using KeyType = typename KeyPortalType::ValueType;

  VTKM_STATIC_ASSERT((std::is_same<typename IdPortalType::ValueType, vtkm::Id>::value));
  VTKM_STATIC_ASSERT(
    (std::is_same<typename IdComponentPortalType::ValueType, vtkm::IdComponent>::value));

  KeyPortalType UniqueKeys;
  IdPortalType SortedValuesMap;
  IdPortalType Offsets;
  IdComponentPortalType Counts;

  VTKM_EXEC_CONT
  ReduceByKeyLookup(const KeyPortalType& uniqueKeys,
                    const IdPortalType& sortedValuesMap,
                    const IdPortalType& offsets,
                    const IdComponentPortalType& counts)
    : UniqueKeys(uniqueKeys)
    , SortedValuesMap(sortedValuesMap)
    , Offsets(offsets)
    , Counts(counts)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ReduceByKeyLookup() {}
};
}
}
} // namespace vtkm::exec::internal

#endif //vtk_m_exec_internal_ReduceByKeyLookup_h
