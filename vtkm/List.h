//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_List_h
#define vtk_m_List_h

#include <vtkm/Types.h>

#include <vtkm/internal/brigand.hpp>

namespace vtkm
{

template <typename... Ts>
struct List
{
};

namespace detail
{

// This prototype is here to detect deprecated ListTag objects. When ListTags are removed, then
// this should be removed too.
struct ListRoot;
}

namespace internal
{

template <typename T>
struct IsListImpl
{
  // This prototype is here to detect deprecated ListTag objects. When ListTags are removed, then
  // this should be changed to be just std::false_type.
  using type = std::is_base_of<vtkm::detail::ListRoot, T>;
};

template <typename... Ts>
struct IsListImpl<vtkm::List<Ts...>>
{
  using type = std::true_type;
};

template <typename T>
using IsList = typename vtkm::internal::IsListImpl<T>::type;

} // namespace internal

/// Checks that the argument is a proper list. This is a handy concept
/// check for functions and classes to make sure that a template argument is
/// actually a device adapter tag. (You can get weird errors elsewhere in the
/// code when a mistake is made.)
///
#define VTKM_IS_LIST(type)                                                                         \
  VTKM_STATIC_ASSERT_MSG((::vtkm::internal::IsList<type>::value),                                  \
                         "Provided type is not a valid VTK-m list type.")

namespace detail
{

/// list value that is used to represent a list actually matches all values
struct UniversalTypeTag
{
  //We never want this tag constructed, and by deleting the constructor
  //we get an error when trying to use this class with ForEach.
  UniversalTypeTag() = delete;
};

} // namespace detail

namespace internal
{

// This is here so that the old (deprecated) `ListTag`s can convert themselves to the new
// `List` style and be operated on. When that deprecated functionality goes away, we can
// probably remove `AsList` and just operate directly on the `List`s.
template <typename T>
struct AsListImpl;

template <typename... Ts>
struct AsListImpl<vtkm::List<Ts...>>
{
  using type = vtkm::List<Ts...>;
};

template <typename T>
using AsList = typename AsListImpl<T>::type;
}

/// A special tag for an empty list.
///
using ListEmpty = vtkm::List<>;

/// A special tag for a list that represents holding all potential values
///
/// Note: Can not be used with ForEach and some list transforms for obvious reasons.
using ListUniversal = vtkm::List<detail::UniversalTypeTag>;

namespace detail
{

template <typename T, template <typename...> class Target>
struct ListApplyImpl;
template <typename... Ts, template <typename...> class Target>
struct ListApplyImpl<vtkm::List<Ts...>, Target>
{
  using type = Target<Ts...>;
};
// Cannot apply the universal list.
template <template <typename...> class Target>
struct ListApplyImpl<vtkm::ListUniversal, Target>;

} // namespace detail

/// \brief Applies the list of types to a template.
///
/// Given a ListTag and a templated class, returns the class instantiated with the types
/// represented by the ListTag.
///
template <typename List, template <typename...> class Target>
using ListApply = typename detail::ListApplyImpl<internal::AsList<List>, Target>::type;

/// Becomes an std::integral_constant containing the number of types in a list.
///
template <typename List>
using ListSize =
  std::integral_constant<vtkm::IdComponent,
                         vtkm::IdComponent{ brigand::size<internal::AsList<List>>::value }>;

/// \brief Finds the type at the given index.
///
/// This becomes the type of the list at the given index.
///
template <typename List, vtkm::IdComponent Index>
using ListAt =
  brigand::at<internal::AsList<List>, std::integral_constant<vtkm::IdComponent, Index>>;

namespace detail
{

// This find is roughly based on the brigand::find functionality. We don't use brigand::find
// because it has an apparent bug where if a list contains templated types, it trys to
// apply those types as predicates, which is wrong.

template <vtkm::IdComponent NumSearched, typename Target, typename... Remaining>
struct FindFirstOfType;

// Not found
template <vtkm::IdComponent NumSearched, typename Target>
struct FindFirstOfType<NumSearched, Target> : std::integral_constant<vtkm::IdComponent, -1>
{
};

// Basic search next one
template <bool NextIsTarget, vtkm::IdComponent NumSearched, typename Target, typename... Remaining>
struct FindFirstOfCheckHead;

template <vtkm::IdComponent NumSearched, typename Target, typename... Ts>
struct FindFirstOfCheckHead<true, NumSearched, Target, Ts...>
  : std::integral_constant<vtkm::IdComponent, NumSearched>
{
};

template <vtkm::IdComponent NumSearched, typename Target, typename Next, typename... Remaining>
struct FindFirstOfCheckHead<false, NumSearched, Target, Next, Remaining...>
  : FindFirstOfCheckHead<std::is_same<Target, Next>::value, NumSearched + 1, Target, Remaining...>
{
};

// Not found
template <vtkm::IdComponent NumSearched, typename Target>
struct FindFirstOfCheckHead<false, NumSearched, Target>
  : std::integral_constant<vtkm::IdComponent, -1>
{
};

template <vtkm::IdComponent NumSearched, typename Target, typename Next, typename... Remaining>
struct FindFirstOfType<NumSearched, Target, Next, Remaining...>
  : FindFirstOfCheckHead<std::is_same<Target, Next>::value, NumSearched, Target, Remaining...>
{
};

// If there are at least 6 entries, check the first 4 to quickly narrow down
template <bool OneInFirst4Matches, vtkm::IdComponent NumSearched, typename Target, typename... Ts>
struct FindFirstOfSplit4;

template <vtkm::IdComponent NumSearched,
          typename Target,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename... Ts>
struct FindFirstOfSplit4<true, NumSearched, Target, T0, T1, T2, T3, Ts...>
  : FindFirstOfCheckHead<std::is_same<Target, T0>::value, NumSearched, Target, T1, T2, T3>
{
};

template <vtkm::IdComponent NumSearched,
          typename Target,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename... Ts>
struct FindFirstOfSplit4<false, NumSearched, Target, T0, T1, T2, T3, T4, Ts...>
  : FindFirstOfCheckHead<std::is_same<Target, T4>::value, NumSearched + 4, Target, Ts...>
{
};

template <vtkm::IdComponent NumSearched,
          typename Target,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename... Ts>
struct FindFirstOfType<NumSearched, Target, T0, T1, T2, T3, T4, T5, Ts...>
  : FindFirstOfSplit4<(std::is_same<Target, T0>::value || std::is_same<Target, T1>::value ||
                       std::is_same<Target, T2>::value ||
                       std::is_same<Target, T3>::value),
                      NumSearched,
                      Target,
                      T0,
                      T1,
                      T2,
                      T3,
                      T4,
                      T5,
                      Ts...>
{
};

// If there are at least 12 entries, check the first 8 to quickly narrow down
template <bool OneInFirst8Matches, vtkm::IdComponent NumSearched, typename Target, typename... Ts>
struct FindFirstOfSplit8;

template <vtkm::IdComponent NumSearched,
          typename Target,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename T6,
          typename T7,
          typename... Ts>
struct FindFirstOfSplit8<true, NumSearched, Target, T0, T1, T2, T3, T4, T5, T6, T7, Ts...>
  : FindFirstOfSplit4<(std::is_same<Target, T0>::value || std::is_same<Target, T1>::value ||
                       std::is_same<Target, T2>::value ||
                       std::is_same<Target, T3>::value),
                      NumSearched,
                      Target,
                      T0,
                      T1,
                      T2,
                      T3,
                      T4,
                      T5,
                      T6,
                      T7>
{
};

template <vtkm::IdComponent NumSearched,
          typename Target,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename T6,
          typename T7,
          typename... Ts>
struct FindFirstOfSplit8<false, NumSearched, Target, T0, T1, T2, T3, T4, T5, T6, T7, Ts...>
  : FindFirstOfType<NumSearched + 8, Target, Ts...>
{
};

template <vtkm::IdComponent NumSearched,
          typename Target,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename T6,
          typename T7,
          typename T8,
          typename T9,
          typename T10,
          typename T11,
          typename... Ts>
struct FindFirstOfType<NumSearched, Target, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, Ts...>
  : FindFirstOfSplit8<(std::is_same<Target, T0>::value || std::is_same<Target, T1>::value ||
                       std::is_same<Target, T2>::value ||
                       std::is_same<Target, T3>::value ||
                       std::is_same<Target, T4>::value ||
                       std::is_same<Target, T5>::value ||
                       std::is_same<Target, T6>::value ||
                       std::is_same<Target, T7>::value),
                      NumSearched,
                      Target,
                      T0,
                      T1,
                      T2,
                      T3,
                      T4,
                      T5,
                      T6,
                      T7,
                      T8,
                      T9,
                      T10,
                      T11,
                      Ts...>
{
};

template <typename List, typename Target>
struct ListIndexOfImpl;
template <typename... Ts, typename Target>
struct ListIndexOfImpl<vtkm::List<Ts...>, Target>
{
  using type = std::integral_constant<vtkm::IdComponent, FindFirstOfType<0, Target, Ts...>::value>;
};
template <typename Target>
struct ListIndexOfImpl<vtkm::ListUniversal, Target>
{
  VTKM_STATIC_ASSERT_MSG((std::is_same<Target, void>::value && std::is_same<Target, int>::value),
                         "Cannot get indices in a universal list.");
};

} // namespace detail

/// \brief Finds the index of a given type.
///
/// Becomes a `std::integral_constant` for the index of the given type. If the
/// given type is not in the list, the value is set to -1.
///
template <typename List, typename T>
using ListIndexOf = typename detail::ListIndexOfImpl<internal::AsList<List>, T>::type;

namespace detail
{

template <typename List, typename T>
struct ListHasImpl
{
  using type = std::integral_constant<bool, (vtkm::ListIndexOf<List, T>::value >= 0)>;
};

template <typename T>
struct ListHasImpl<vtkm::ListUniversal, T>
{
  using type = std::true_type;
};

} // namespace detail

/// \brief Checks to see if the given `T` is in the list pointed to by `List`.
///
/// Becomes `std::true_type` if the `T` is in `List`. `std::false_type` otherwise.
///
template <typename List, typename T>
using ListHas = typename detail::ListHasImpl<internal::AsList<List>, T>::type;

#if defined(VTKM_MSVC) && (_MSC_VER < 1911)

// Alternate definition of ListAppend to get around an apparent issue with
// Visual Studio 2015.
namespace detail
{

template <typename... Lists>
struct ListAppendImpl
{
  using type = brigand::append<internal::AsList<Lists>...>;
};

} // namespace detail

template <typename... Lists>
using ListAppend = typename detail::ListAppendImpl<Lists...>::type;

#else // Normal definition

/// Concatinates a set of lists into a single list.
///
/// Note that this does not work correctly with `vtkm::ListUniversal`.
template <typename... Lists>
using ListAppend = brigand::append<internal::AsList<Lists>...>;

#endif

namespace detail
{

template <bool Has, typename State, typename Element>
struct ListIntersectTagsChoose;

template <typename State, typename Element>
struct ListIntersectTagsChoose<true, State, Element>
{
  using type = brigand::push_back<State, Element>;
};

template <typename State, typename Element>
struct ListIntersectTagsChoose<false, State, Element>
{
  using type = State;
};

template <class State, class Element, class List>
struct ListIntersectTags
  : ListIntersectTagsChoose<vtkm::ListHas<List, Element>::value, State, Element>
{
};

template <typename List1, typename List2>
struct ListIntersectImpl
{
  VTKM_IS_LIST(List1);
  VTKM_IS_LIST(List2);

  using type =
    brigand::fold<List1,
                  vtkm::List<>,
                  ListIntersectTags<brigand::_state, brigand::_element, brigand::pin<List2>>>;
};

template <typename List1>
struct ListIntersectImpl<List1, vtkm::ListUniversal>
{
  VTKM_IS_LIST(List1);

  using type = List1;
};
template <typename List2>
struct ListIntersectImpl<vtkm::ListUniversal, List2>
{
  VTKM_IS_LIST(List2);

  using type = List2;
};
template <>
struct ListIntersectImpl<vtkm::ListUniversal, vtkm::ListUniversal>
{
  using type = vtkm::ListUniversal;
};

} // namespace detail

/// Constructs a list containing types present in all lists.
///
template <typename List1, typename List2>
using ListIntersect =
  typename detail::ListIntersectImpl<internal::AsList<List1>, internal::AsList<List2>>::type;

namespace detail
{

template <typename T, template <typename> class Target>
struct ListTransformImpl;
template <typename... Ts, template <typename> class Target>
struct ListTransformImpl<vtkm::List<Ts...>, Target>
{
  using type = vtkm::List<Target<Ts>...>;
};
// Cannot transform the universal list.
template <template <typename> class Target>
struct ListTransformImpl<vtkm::ListUniversal, Target>;

} // namespace detail

/// Constructs a list containing all types in a source list applied to a transform template.
///
template <typename List, template <typename> class Transform>
using ListTransform = typename detail::ListTransformImpl<internal::AsList<List>, Transform>::type;

/// Takes an existing `List` and a predicate template that is applied to each type in the `List`.
/// Any type in the `List` that has a value element equal to true (the equivalent of
/// `std::true_type`), that item will be removed from the list. For example the following type
///
/// ```cpp
/// vtkm::ListRemoveIf<vtkm::List<int, float, long long, double>, std::is_integral>
/// ```
///
/// resolves to a `List` that is equivalent to `vtkm::List<float, double>` because
/// `std::is_integral<int>` and `std::is_integral<long long>` resolve to `std::true_type` whereas
/// `std::is_integral<float>` and `std::is_integral<double>` resolve to `std::false_type`.
///
template <typename List, template <typename> class Predicate>
using ListRemoveIf =
  brigand::remove_if<internal::AsList<List>, brigand::bind<Predicate, brigand::_1>>;

namespace detail
{

// We want to use an initializer list as a trick to call a function once for each type, but
// an initializer list needs a type, so create wrapper function that returns a value.
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename Functor, typename... Args>
VTKM_EXEC_CONT inline bool ListForEachCallThrough(Functor&& f, Args&&... args)
{
  f(std::forward<Args>(args)...);
  return false; // Return value does not matter. Hopefully just thrown away.
}

VTKM_SUPPRESS_EXEC_WARNINGS
template <typename Functor, typename... Ts, typename... Args>
VTKM_EXEC_CONT void ListForEachImpl(Functor&& f, vtkm::List<Ts...>, Args&&... args)
{
  auto init_list = { ListForEachCallThrough(
    std::forward<Functor>(f), Ts{}, std::forward<Args>(args)...)... };
  (void)init_list;
}

template <typename Functor, typename... Args>
VTKM_EXEC_CONT void ListForEachImpl(Functor&&, vtkm::ListEmpty, Args&&...)
{
  // No types to run functor on.
}

} // namespace detail

/// For each typename represented by the list, call the functor with a
/// default instance of that type.
///
template <typename Functor, typename List, typename... Args>
VTKM_EXEC_CONT void ListForEach(Functor&& f, List, Args&&... args)
{
  detail::ListForEachImpl(
    std::forward<Functor>(f), internal::AsList<List>{}, std::forward<Args>(args)...);
}

namespace detail
{

template <typename List1, typename List2>
struct ListCrossImpl
{
  VTKM_IS_LIST(List1);
  VTKM_IS_LIST(List2);

  // This is a lazy Cartesian product generator.
  // This version was settled on as being the best default
  // version as all compilers including Intel handle this
  // implementation without issue for very large cross products
  using type = brigand::reverse_fold<
    vtkm::List<List1, List2>,
    vtkm::List<vtkm::List<>>,
    brigand::lazy::join<brigand::lazy::transform<
      brigand::_2,
      brigand::defer<brigand::lazy::join<brigand::lazy::transform<
        brigand::parent<brigand::_1>,
        brigand::defer<brigand::bind<
          vtkm::List,
          brigand::lazy::push_front<brigand::_1, brigand::parent<brigand::_1>>>>>>>>>>;
};

} // namespace detail

/// \brief Generates a list that is the cross product of two input lists.
///
/// The resulting list has the form of `vtkm::List<vtkm::List<A1,B1>, vtkm::List<A1,B2>,...>`
///
template <typename List1, typename List2>
using ListCross =
  typename detail::ListCrossImpl<internal::AsList<List1>, internal::AsList<List2>>::type;

} // namespace vtkm

#endif //vtk_m_List_h
