//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_Hints_h
#define vtk_m_cont_internal_Hints_h

#include <vtkm/Assert.h>
#include <vtkm/List.h>

#include <vtkm/cont/DeviceAdapterTag.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// @brief Representation of a hint for execution.
///
/// A hint is a (potentially) device independent parameter that can be used when
/// scheduling parallel execution on a device. Control-side code can provide hints
/// when scheduling parallel device execution to provide some context about what
/// is being run and potentially optimize the algorithm. An implementation for
/// a device adapter can choose to use or ignore hints. Likewise, a hint can be
/// attached to a specific list of devices.
///
/// This base class is not intended to be used directly. Use one of the
/// derived hint structures to specify a hint.
template <typename Derived_, typename Tag_, typename DeviceList_>
struct HintBase
{
  using Derived = Derived_;
  using Tag = Tag_;
  using DeviceList = DeviceList_;
};

struct HintTagThreadsPerBlock
{
};

/// @brief Suggest the number of threads to use when scheduling blocks of threads.
///
/// Many accelerator devices, particularly GPUs, schedule threads in blocks. This
/// hint suggests the size of block to use during the scheduling.
template <vtkm::IdComponent MaxThreads_, typename DeviceList_ = vtkm::ListUniversal>
struct HintThreadsPerBlock
  : HintBase<HintThreadsPerBlock<MaxThreads_, DeviceList_>, HintTagThreadsPerBlock, DeviceList_>
{
  static constexpr vtkm::IdComponent MaxThreads = MaxThreads_;
};

/// @brief Container for hints.
///
/// When scheduling or invoking a parallel routine, the caller can provide a list
/// of hints to suggest the best way to execute the routine. This list is provided
/// as arguments to a `HintList` template and passed as an argument.
template <typename... Hints>
struct HintList : vtkm::List<Hints...>
{
  using List = vtkm::List<Hints...>;
};

template <typename T>
struct IsHintList : std::false_type
{
};
template <typename... Hints>
struct IsHintList<HintList<Hints...>> : std::true_type
{
};

/// @brief Performs a static assert that the given object is a hint list.
///
/// If the provided type is a `vtkm::cont::internal::HintList`, then this macro
/// does nothing. If the type is anything else, a compile error will occur. This
/// macro is useful for checking that template arguments are an expected hint
/// list. This helps diagnose improper template use more easily.
#define VTKM_IS_HINT_LIST(T) VTKM_STATIC_ASSERT(::vtkm::cont::internal::IsHintList<T>::value)

namespace detail
{

template <typename Device, typename HintTag>
struct FindHintOperators
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  template <typename Hint>
  using HintMatches = vtkm::internal::meta::And<std::is_same<typename Hint::Tag, HintTag>,
                                                vtkm::ListHas<typename Hint::DeviceList, Device>>;
  template <typename Found, typename Next>
  using ReduceOperator = typename std::conditional<HintMatches<Next>::value, Next, Found>::type;
};

} // namespace detail

/// @brief Find a hint of a particular type.
///
/// The `HintFind` template can be used to find a hint of a particular type.
/// `HintFind` is provided a default value to use for a hint, and it returns
/// a hint in the hint list that matches the type of the provided default and
/// applies to the provided device tag.
///
/// If multiple hints match the type and device, the _last_ one in the list
/// is returned. Thus, when constructing hint lists, but the more general hints
/// first and more specific ones last.
template <typename HList, typename DefaultHint, typename Device>
using HintFind = vtkm::ListReduce<
  typename HList::List,
  detail::FindHintOperators<Device, typename DefaultHint::Tag>::template ReduceOperator,
  DefaultHint>;

}
}
} // namespace vtkm::cont::internal

#endif // vtk_m_cont_internal_Hints_h
