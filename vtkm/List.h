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

#include <vtkm/internal/Meta.h>

#include <functional>

namespace vtkm
{

// We are currently limiting the size of a `vtkm::List`. Generally, a compiler will
// happily create a list of any size. However, to make sure the compiler does not go
// into an infinite loop, you can only iterate on a list so far. As such, it is safer
// to limit the size of the lists.
#define VTKM_CHECK_LIST_SIZE(size)                                                               \
  static_assert((size) <= 512,                                                                   \
                "A vtkm::List with more than 512 elements is not supported."                     \
                " A list this long is problematic for compilers."                                \
                " Compilers often have a recursive template instantiation limit of around 1024," \
                " so operations on lists this large can lead to confusing and misleading errors.")

template <typename... Ts>
struct List
{
  VTKM_CHECK_LIST_SIZE(sizeof...(Ts));
};

namespace internal
{

template <typename T>
struct IsListImpl
{
  using type = std::false_type;
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
#define VTKM_IS_LIST(type)                                        \
  VTKM_STATIC_ASSERT_MSG((::vtkm::internal::IsList<type>::value), \
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

/// A special tag for an empty list.
///
using ListEmpty = vtkm::List<>;

/// A special tag for a list that represents holding all potential values
///
/// Note: Can not be used with ForEach and some list transforms for obvious reasons.
using ListUniversal = vtkm::List<detail::UniversalTypeTag>;

namespace detail
{

template <typename L>
struct ListSizeImpl;

template <typename... Ts>
struct ListSizeImpl<vtkm::List<Ts...>>
{
  using type =
    std::integral_constant<vtkm::IdComponent, static_cast<vtkm::IdComponent>(sizeof...(Ts))>;
};

} // namespace detail

/// Becomes an std::integral_constant containing the number of types in a list.
///
template <typename List>
using ListSize = typename detail::ListSizeImpl<List>::type;

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
using ListApply = typename detail::ListApplyImpl<List, Target>::type;

namespace detail
{

template <typename... Ls>
struct ListAppendImpl;

template <>
struct ListAppendImpl<>
{
  using type = vtkm::ListEmpty;
};

template <typename L>
struct ListAppendImpl<L>
{
  using type = L;
};

template <typename... T0s, typename... T1s>
struct ListAppendImpl<vtkm::List<T0s...>, vtkm::List<T1s...>>
{
  using type = vtkm::List<T0s..., T1s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s, typename... T1s, typename... T2s>
struct ListAppendImpl<vtkm::List<T0s...>, vtkm::List<T1s...>, vtkm::List<T2s...>>
{
  using type = vtkm::List<T0s..., T1s..., T2s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s, typename... T1s, typename... T2s, typename... T3s>
struct ListAppendImpl<vtkm::List<T0s...>,
                      vtkm::List<T1s...>,
                      vtkm::List<T2s...>,
                      vtkm::List<T3s...>>
{
  using type = vtkm::List<T0s..., T1s..., T2s..., T3s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s, typename... T1s, typename... T2s, typename... T3s, typename... T4s>
struct ListAppendImpl<vtkm::List<T0s...>,
                      vtkm::List<T1s...>,
                      vtkm::List<T2s...>,
                      vtkm::List<T3s...>,
                      vtkm::List<T4s...>>
{
  using type = vtkm::List<T0s..., T1s..., T2s..., T3s..., T4s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s,
          typename... T1s,
          typename... T2s,
          typename... T3s,
          typename... T4s,
          typename... T5s>
struct ListAppendImpl<vtkm::List<T0s...>,
                      vtkm::List<T1s...>,
                      vtkm::List<T2s...>,
                      vtkm::List<T3s...>,
                      vtkm::List<T4s...>,
                      vtkm::List<T5s...>>
{
  using type = vtkm::List<T0s..., T1s..., T2s..., T3s..., T4s..., T5s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s,
          typename... T1s,
          typename... T2s,
          typename... T3s,
          typename... T4s,
          typename... T5s,
          typename... T6s>
struct ListAppendImpl<vtkm::List<T0s...>,
                      vtkm::List<T1s...>,
                      vtkm::List<T2s...>,
                      vtkm::List<T3s...>,
                      vtkm::List<T4s...>,
                      vtkm::List<T5s...>,
                      vtkm::List<T6s...>>
{
  using type = vtkm::List<T0s..., T1s..., T2s..., T3s..., T4s..., T5s..., T6s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s,
          typename... T1s,
          typename... T2s,
          typename... T3s,
          typename... T4s,
          typename... T5s,
          typename... T6s,
          typename... T7s>
struct ListAppendImpl<vtkm::List<T0s...>,
                      vtkm::List<T1s...>,
                      vtkm::List<T2s...>,
                      vtkm::List<T3s...>,
                      vtkm::List<T4s...>,
                      vtkm::List<T5s...>,
                      vtkm::List<T6s...>,
                      vtkm::List<T7s...>>
{
  using type = vtkm::List<T0s..., T1s..., T2s..., T3s..., T4s..., T5s..., T6s..., T7s...>;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

template <typename... T0s,
          typename... T1s,
          typename... T2s,
          typename... T3s,
          typename... T4s,
          typename... T5s,
          typename... T6s,
          typename... T7s,
          typename... Ls>
struct ListAppendImpl<vtkm::List<T0s...>,
                      vtkm::List<T1s...>,
                      vtkm::List<T2s...>,
                      vtkm::List<T3s...>,
                      vtkm::List<T4s...>,
                      vtkm::List<T5s...>,
                      vtkm::List<T6s...>,
                      vtkm::List<T7s...>,
                      Ls...>
{
  using type = typename ListAppendImpl<
    vtkm::List<T0s..., T1s..., T2s..., T3s..., T4s..., T5s..., T6s..., T7s...>,
    Ls...>::type;
  VTKM_CHECK_LIST_SIZE(vtkm::ListSize<type>::value);
};

} // namespace detail

/// Concatinates a set of lists into a single list.
///
/// Note that this does not work correctly with `vtkm::ListUniversal`.
template <typename... Lists>
using ListAppend = typename detail::ListAppendImpl<Lists...>::type;

namespace detail
{

template <typename T, vtkm::IdComponent N>
struct ListFillImpl
{
  using type = typename ListAppendImpl<typename ListFillImpl<T, (N / 2)>::type,
                                       typename ListFillImpl<T, (N - (N / 2))>::type>::type;
};

template <typename T>
struct ListFillImpl<T, 1>
{
  using type = vtkm::List<T>;
};

template <typename T>
struct ListFillImpl<T, 0>
{
  using type = vtkm::List<>;
};

} // namespace detail

/// \brief Returns a list filled with N copies of type T
///
template <typename T, vtkm::IdComponent N>
using ListFill = typename detail::ListFillImpl<T, N>::type;

namespace detail
{

template <typename T>
struct ListAtImplFunc;

template <typename... VoidTypes>
struct ListAtImplFunc<vtkm::List<VoidTypes...>>
{
  // Rather than declare a `type`, make a declaration of a function that returns the type
  // after some number of `const void*` arguments. We can use ListFill to quickly create
  // the list of `const void*` arguments, so the type can be returned. We can then use
  // decltype to get the returned type.
  //
  // Templating the `Other` should not be strictly necessary. (You should be able to just
  // end with `...`.) But some compilers (such as CUDA 8 and Intel 18) have a problem with
  // that.
  template <typename T, class... Other>
  static T at(VoidTypes..., T*, Other...);
};

template <typename T, vtkm::IdComponent Index>
struct ListAtImpl;

template <typename... Ts, vtkm::IdComponent Index>
struct ListAtImpl<vtkm::List<Ts...>, Index>
{
  using type =
    decltype(ListAtImplFunc<vtkm::ListFill<const void*, Index>>::at(static_cast<Ts*>(nullptr)...));
};

} // namespace detail

/// \brief Finds the type at the given index.
///
/// This becomes the type of the list at the given index.
///
template <typename List, vtkm::IdComponent Index>
using ListAt = typename detail::ListAtImpl<List, Index>::type;

namespace detail
{

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
                       std::is_same<Target, T2>::value || std::is_same<Target, T3>::value),
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
                       std::is_same<Target, T2>::value || std::is_same<Target, T3>::value),
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
                       std::is_same<Target, T2>::value || std::is_same<Target, T3>::value ||
                       std::is_same<Target, T4>::value || std::is_same<Target, T5>::value ||
                       std::is_same<Target, T6>::value || std::is_same<Target, T7>::value),
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
using ListIndexOf = typename detail::ListIndexOfImpl<List, T>::type;

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
using ListHas = typename detail::ListHasImpl<List, T>::type;

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
using ListTransform = typename detail::ListTransformImpl<List, Transform>::type;

namespace detail
{

#if defined(VTKM_MSVC) && (_MSC_VER < 1920)
// Special (inefficient) implementation for MSVC 2017, which has an ICE for the regular version

template <typename Passed,
          typename Next,
          bool Remove,
          typename Rest,
          template <typename>
          class Predicate>
struct ListRemoveIfCheckNext;

template <typename Passed, typename Rest, template <typename> class Predicate>
struct ListRemoveIfGetNext;

template <typename Passed, typename Next, typename Rest, template <typename> class Predicate>
struct ListRemoveIfCheckNext<Passed, Next, true, Rest, Predicate>
{
  using type = typename ListRemoveIfGetNext<Passed, Rest, Predicate>::type;
};

template <typename... PassedTs, typename Next, typename Rest, template <typename> class Predicate>
struct ListRemoveIfCheckNext<vtkm::List<PassedTs...>, Next, false, Rest, Predicate>
{
  using type = typename ListRemoveIfGetNext<vtkm::List<PassedTs..., Next>, Rest, Predicate>::type;
};

template <typename Passed, typename Next, typename... RestTs, template <typename> class Predicate>
struct ListRemoveIfGetNext<Passed, vtkm::List<Next, RestTs...>, Predicate>
{
  using type = typename ListRemoveIfCheckNext<Passed,
                                              Next,
                                              Predicate<Next>::value,
                                              vtkm::List<RestTs...>,
                                              Predicate>::type;
};

template <typename Passed, template <typename> class Predicate>
struct ListRemoveIfGetNext<Passed, vtkm::List<>, Predicate>
{
  using type = Passed;
};

template <typename L, template <typename> class Predicate>
struct ListRemoveIfImpl
{
  using type = typename ListRemoveIfGetNext<vtkm::List<>, L, Predicate>::type;
};

#else

template <typename L, template <typename> class Predicate>
struct ListRemoveIfImpl;

template <typename... Ts, template <typename> class Predicate>
struct ListRemoveIfImpl<vtkm::List<Ts...>, Predicate>
{
  using type = typename ListAppendImpl<
    std::conditional_t<Predicate<Ts>::value, vtkm::List<>, vtkm::List<Ts>>...>::type;
};

#endif

} // namespace detail

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
using ListRemoveIf = typename detail::ListRemoveIfImpl<List, Predicate>::type;

namespace detail
{

template <typename List1, typename List2>
struct ListIntersectImpl
{
  template <typename T>
  struct Predicate
  {
    static constexpr bool value = !vtkm::ListHas<List1, T>::value;
  };

  using type = vtkm::ListRemoveIf<List2, Predicate>;
};

template <typename List1>
struct ListIntersectImpl<List1, vtkm::ListUniversal>
{
  using type = List1;
};
template <typename List2>
struct ListIntersectImpl<vtkm::ListUniversal, List2>
{
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
using ListIntersect = typename detail::ListIntersectImpl<List1, List2>::type;

///@{
/// For each typename represented by the list, call the functor with a
/// default instance of that type.
///
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename Functor, typename... Ts, typename... Args>
VTKM_EXEC_CONT void ListForEach(Functor&& f, vtkm::List<Ts...>, Args&&... args)
{
  VTKM_STATIC_ASSERT_MSG((!std::is_same<vtkm::List<Ts...>, vtkm::ListUniversal>::value),
                         "Cannot call ListFor on vtkm::ListUniversal.");
  auto init_list = { (f(Ts{}, std::forward<Args>(args)...), false)... };
  (void)init_list;
}
template <typename Functor, typename... Args>
VTKM_EXEC_CONT void ListForEach(Functor&&, vtkm::ListEmpty, Args&&...)
{
  // No types to run functor on.
}
///@}

namespace detail
{

template <typename List1, typename List2>
struct ListCrossImpl;

template <typename... T0s, typename... T1s>
struct ListCrossImpl<vtkm::List<T0s...>, vtkm::List<T1s...>>
{
  template <typename T>
  struct Predicate
  {
    using type = vtkm::List<vtkm::List<T, T1s>...>;
  };

  using type = vtkm::ListAppend<typename Predicate<T0s>::type...>;
};

} // namespace detail

/// \brief Generates a list that is the cross product of two input lists.
///
/// The resulting list has the form of `vtkm::List<vtkm::List<A1,B1>, vtkm::List<A1,B2>,...>`
///
template <typename List1, typename List2>
using ListCross = typename detail::ListCrossImpl<List1, List2>::type;

namespace detail
{

template <typename L, template <typename T1, typename T2> class Operator, typename Result>
struct ListReduceImpl;

template <template <typename T1, typename T2> class Operator, typename Result>
struct ListReduceImpl<vtkm::List<>, Operator, Result>
{
  using type = Result;
};

template <typename T0,
          typename... Ts,
          template <typename O1, typename O2>
          class Operator,
          typename Result>
struct ListReduceImpl<vtkm::List<T0, Ts...>, Operator, Result>
{
  using type = typename ListReduceImpl<vtkm::List<Ts...>, Operator, Operator<Result, T0>>::type;
};

template <typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename T6,
          typename T7,
          typename T8,
          typename... Ts,
          template <typename O1, typename O2>
          class Operator,
          typename Result>
struct ListReduceImpl<vtkm::List<T0, T1, T2, T3, T4, T5, T6, T7, T8, Ts...>, Operator, Result>
{
  using type = typename ListReduceImpl<
    vtkm::List<T8, Ts...>,
    Operator,
    typename ListReduceImpl<vtkm::List<T0, T1, T2, T3, T4, T5, T6, T7>, Operator, Result>::type>::
    type;
};

} // namespace detail

/// \brief Reduces a list to a single type using an operator.
///
/// `ListReduce` takes a `vtkm::List`, an operator template, and an initial type. `ListReduce`
/// first applies the initial type and the first item in the list to the operator and gets
/// that type. That then applies the operator to that result and the next item in the list.
/// This continues until a single value is left.
///
template <typename List, template <typename T1, typename T2> class Operator, typename Initial>
using ListReduce = typename detail::ListReduceImpl<List, Operator, Initial>::type;

/// \brief Determines whether all the types in the list are "true."
///
/// `ListAll` expects a `vtkm::List` with types that have a `value` that is either true or false
/// (such as `std::true_type` and `std::false_type`. Resolves to `std::true_type` if all the types
/// are true, `std::false_type` otherwise. If the list is empty, resolves to `std::true_type`.
///
/// `ListAll` also accepts an optional second argument that is a template that is a predicate
/// applied to each item in the input list before checking for the `value` type.
///
/// ```cpp
/// using NumberList1 = vtkm::List<int, char>;
/// using NumberList2 = vtkm::List<int, float>;
///
/// // Resolves to std::true_type
/// using AllInt1 = vtkm::ListAll<NumberList1, std::is_integral>;
///
/// // Resolves to std::false_type (because float is not integral)
/// using AllInt2 = vtkm::ListAll<NumberList2, std::is_integral>;
/// ```
///
template <typename List, template <typename> class Predicate = vtkm::internal::meta::Identity>
using ListAll =
  vtkm::ListReduce<vtkm::ListTransform<List, Predicate>, vtkm::internal::meta::And, std::true_type>;

/// \brief Determines whether any of the types in the list are "true."
///
/// `ListAny` expects a `vtkm::List` with types that have a `value` that is either true or false
/// (such as `std::true_type` and `std::false_type`. Resolves to `std::true_type` if any of the
/// types are true, `std::false_type` otherwise. If the list is empty, resolves to
/// `std::false_type`.
///
/// `ListAny` also accepts an optional second argument that is a template that is a predicate
/// applied to each item in the input list before checking for the `value` type.
///
/// ```cpp
/// using NumberList1 = vtkm::List<int, float>;
/// using NumberList2 = vtkm::List<float, double>;
///
/// // Resolves to std::true_type
/// using AnyInt1 = vtkm::ListAny<NumberList1, std::is_integral>;
///
/// // Resolves to std::false_type
/// using AnyInt2 = vtkm::ListAny<NumberList2, std::is_integral>;
/// ```
///
template <typename List, template <typename> class Predicate = vtkm::internal::meta::Identity>
using ListAny =
  vtkm::ListReduce<vtkm::ListTransform<List, Predicate>, vtkm::internal::meta::Or, std::false_type>;

#undef VTKM_CHECK_LIST_SIZE

} // namespace vtkm

#endif //vtk_m_List_h
