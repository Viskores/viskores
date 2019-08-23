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
#ifndef SimpleTuple_h
#define SimpleTuple_h

// A simple tuple implementation for simple compilers.
//
// Most platforms use the taocpp/tuple implementation in thirdparty/taotuple,
// but not all are capable of handling the metaprogramming techniques used.
// This simple recursion-based tuple implementation is used where tao fails.

#include <type_traits>
#include <utility>

#ifndef TAOCPP_ANNOTATION
#ifdef __CUDACC__
#define TAOCPP_ANNOTATION __host__ __device__
#else
#define TAOCPP_ANNOTATION
#endif // __CUDACC__
#endif // TAOCPP_ANNOTATION

// Ignore "calling a __host__ function from a __host__ _device__ function is not allowed" warnings
#ifndef TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
#ifdef __CUDACC__
#if __CUDAVER__ >= 75000
#define TAO_TUPLE_SUPPRESS_NVCC_HD_WARN #pragma nv_exec_check_disable
#else
#define TAO_TUPLE_SUPPRESS_NVCC_HD_WARN #pragma hd_warning_disable
#endif
#else
#define TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
#endif
#endif


namespace simple_tuple
{
namespace detail
{

template <std::size_t Index, typename Head>
class tuple_leaf
{
protected:
  Head Value;

public:
  TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
  TAOCPP_ANNOTATION constexpr tuple_leaf()
    : Value()
  {
  }

  TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
  TAOCPP_ANNOTATION constexpr tuple_leaf(const Head& value)
    : Value(value)
  {
  }

  TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
  TAOCPP_ANNOTATION constexpr tuple_leaf(const tuple_leaf& o)
    : Value(o.Value)
  {
  }

  TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
  TAOCPP_ANNOTATION constexpr tuple_leaf(tuple_leaf&& o)
    : Value(std::move(o.Value))
  {
  }

  TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
  template <typename Other>
  TAOCPP_ANNOTATION constexpr tuple_leaf(Other&& o)
    : Value(std::forward<Other>(o))
  {
  }

  static TAOCPP_ANNOTATION constexpr Head& Get(tuple_leaf& o) noexcept { return o.Value; }

  static TAOCPP_ANNOTATION constexpr const Head& Get(const tuple_leaf& o) noexcept
  {
    return o.Value;
  }

  TAO_TUPLE_SUPPRESS_NVCC_HD_WARN
  TAOCPP_ANNOTATION
  tuple_leaf& operator=(tuple_leaf& o)
  {
    this->Value = o.Value;
    return *this;
  }
};

template <std::size_t Index, typename... Ts>
class tuple_impl;

template <std::size_t Idx, typename HeadT, typename... TailTs>
class tuple_impl<Idx, HeadT, TailTs...> : public tuple_impl<Idx + 1, TailTs...>,
                                          private tuple_leaf<Idx, HeadT>
{
public:
  using Tail = tuple_impl<Idx + 1, TailTs...>;
  using Leaf = tuple_leaf<Idx, HeadT>;
  using Head = HeadT;
  static const std::size_t Index = Idx;

  TAOCPP_ANNOTATION constexpr tuple_impl()
    : Tail()
    , Leaf()
  {
  }

  explicit TAOCPP_ANNOTATION constexpr tuple_impl(const HeadT& h, const TailTs&... ts)
    : Tail(ts...)
    , Leaf(h)
  {
  }

  // The enable_if is needed to ensure that tail lengths match (otherwise empty
  // constructors would be called).
  template <typename OHeadT,
            typename... OTailTs,
            typename = typename std::enable_if<sizeof...(TailTs) == sizeof...(OTailTs)>::type>
  explicit TAOCPP_ANNOTATION constexpr tuple_impl(OHeadT&& h, OTailTs&&... ts)
    : Tail(std::forward<OTailTs>(ts)...)
    , Leaf(std::forward<OHeadT>(h))
  {
  }

  constexpr tuple_impl(const tuple_impl&) = default;

  TAOCPP_ANNOTATION constexpr tuple_impl(tuple_impl&& o)
    : Tail(std::move(GetTail(o)))
    , Leaf(std::forward<Head>(GetHead(o)))
  {
  }

  template <typename... Ts>
  TAOCPP_ANNOTATION constexpr tuple_impl(const tuple_impl<Idx, Ts...>& o)
    : Tail(tuple_impl<Idx, Ts...>::GetTail(o))
    , Leaf(tuple_impl<Idx, Ts...>::GetHead(o))
  {
  }

  template <typename OHead, typename... OTailTs>
  TAOCPP_ANNOTATION constexpr tuple_impl(tuple_impl<Idx, OHead, OTailTs...>&& o)
    : Tail(std::move(tuple_impl<Idx, OHead, OTailTs...>::GetTail(o)))
    , Leaf(std::forward<OHead>(tuple_impl<Idx, OHead, OTailTs...>::GetHead(o)))
  {
  }

  TAOCPP_ANNOTATION
  tuple_impl& operator=(const tuple_impl& o)
  {
    GetHead(*this) = GetHead(o);
    GetTail(*this) = GetTail(o);
    return *this;
  }

  TAOCPP_ANNOTATION
  tuple_impl& operator=(tuple_impl&& o)
  {
    GetHead(*this) = std::forward<Head>(GetHead(o));
    GetTail(*this) = std::move(GetTail(o));
    return *this;
  }

  template <typename... Ts>
  TAOCPP_ANNOTATION tuple_impl& operator=(const tuple_impl<Idx, Ts...>& o)
  {
    GetHead(*this) = tuple_impl<Idx, Ts...>::GetHead(o);
    GetTail(*this) = tuple_impl<Idx, Ts...>::GetTail(o);
    return *this;
  }

  template <typename OHead, typename... OTailTs>
  TAOCPP_ANNOTATION tuple_impl& operator=(tuple_impl<Idx, OHead, OTailTs...>&& o)
  {
    using OtherImpl = tuple_impl<Idx, OHead, OTailTs...>;
    GetHead(*this) = std::forward<OHead>(OtherImpl::GetHead(o));
    GetTail(*this) = std::move(OtherImpl::GetTail(o));
    return *this;
  }

  static TAOCPP_ANNOTATION constexpr Head& GetHead(tuple_impl& o) noexcept
  {
    return Leaf::Get(static_cast<Leaf&>(o));
  }

  static TAOCPP_ANNOTATION constexpr const Head& GetHead(const tuple_impl& o) noexcept
  {
    return Leaf::Get(static_cast<const Leaf&>(o));
  }

  static TAOCPP_ANNOTATION constexpr Tail& GetTail(tuple_impl& o) noexcept
  {
    return static_cast<Tail&>(o);
  }

  static TAOCPP_ANNOTATION constexpr const Tail& GetTail(const tuple_impl& o) noexcept
  {
    return static_cast<const Tail&>(o);
  }
};


template <std::size_t Idx, typename HeadT>
class tuple_impl<Idx, HeadT> : private tuple_leaf<Idx, HeadT>
{
public:
  using Leaf = tuple_leaf<Idx, HeadT>;
  using Head = HeadT;
  static const std::size_t Index = Idx;

  TAOCPP_ANNOTATION constexpr tuple_impl()
    : Leaf()
  {
  }

  explicit TAOCPP_ANNOTATION constexpr tuple_impl(const HeadT& h)
    : Leaf(h)
  {
  }

  template <typename OHeadT>
  explicit TAOCPP_ANNOTATION constexpr tuple_impl(OHeadT&& h)
    : Leaf(std::forward<OHeadT>(h))
  {
  }

  TAOCPP_ANNOTATION constexpr tuple_impl(const tuple_impl& o)
    : Leaf(GetHead(o))
  {
  }

  TAOCPP_ANNOTATION constexpr tuple_impl(tuple_impl&& o)
    : Leaf(std::forward<Head>(GetHead(o)))
  {
  }

  template <typename OHeadT>
  TAOCPP_ANNOTATION constexpr tuple_impl(const tuple_impl<Idx, OHeadT>& o)
    : Leaf(tuple_impl<Idx, OHeadT>::GetHead(o))
  {
  }

  template <typename OHeadT>
  TAOCPP_ANNOTATION constexpr tuple_impl(tuple_impl<Idx, OHeadT>&& o)
    : Leaf(std::forward<OHeadT>(tuple_impl<Idx, OHeadT>::GetHead(o)))
  {
  }

  TAOCPP_ANNOTATION
  tuple_impl& operator=(const tuple_impl& o)
  {
    GetHead(*this) = GetHead(o);
    return *this;
  }

  TAOCPP_ANNOTATION
  tuple_impl& operator=(tuple_impl&& o)
  {
    GetHead(*this) = std::forward<Head>(GetHead(o));
    return *this;
  }

  template <typename OHeadT>
  TAOCPP_ANNOTATION tuple_impl& operator=(const tuple_impl<Idx, OHeadT>& o)
  {
    GetHead(*this) = tuple_impl<Idx, OHeadT>::GetHead(o);
    return *this;
  }

  template <typename OHeadT>
  TAOCPP_ANNOTATION tuple_impl& operator=(tuple_impl<Idx, OHeadT>&& o)
  {
    using OtherImpl = tuple_impl<Idx, OHeadT>;
    GetHead(*this) = std::forward<OHeadT>(OtherImpl::GetHead(o));
    return *this;
  }

  static TAOCPP_ANNOTATION constexpr Head& GetHead(tuple_impl& o) noexcept
  {
    return Leaf::Get(static_cast<Leaf&>(o));
  }

  static TAOCPP_ANNOTATION constexpr const Head& GetHead(const tuple_impl& o) noexcept
  {
    return Leaf::Get(static_cast<const Leaf&>(o));
  }
};

template <std::size_t Idx, typename Head, typename... Tail>
TAOCPP_ANNOTATION constexpr Head& get_helper(tuple_impl<Idx, Head, Tail...>& t) noexcept
{
  return tuple_impl<Idx, Head, Tail...>::GetHead(t);
}

template <std::size_t Idx, typename Head, typename... Tail>
TAOCPP_ANNOTATION constexpr const Head& get_helper(const tuple_impl<Idx, Head, Tail...>& t) noexcept
{
  return tuple_impl<Idx, Head, Tail...>::GetHead(t);
}

// Unassignable stateless type:
struct ignore_impl
{
  template <class T>
  TAOCPP_ANNOTATION constexpr const ignore_impl& operator=(const T&) const
  {
    return *this;
  }
};

} // end namespace detail

/// Reimplementation of std::tuple with markup for device support.
template <typename... Ts>
class tuple;

template <typename... Ts>
class tuple : public detail::tuple_impl<0, Ts...>
{
  using Impl = detail::tuple_impl<0, Ts...>;

public:
  TAOCPP_ANNOTATION constexpr tuple()
    : Impl()
  {
  }

  TAOCPP_ANNOTATION constexpr explicit tuple(const Ts&... ts)
    : Impl(ts...)
  {
  }

  template <typename... OTs>
  TAOCPP_ANNOTATION constexpr explicit tuple(const OTs&... ts)
    : Impl(ts...)
  {
  }

  template <typename... OTs>
  TAOCPP_ANNOTATION constexpr explicit tuple(OTs&&... ts)
    : Impl(std::forward<OTs>(ts)...)
  {
  }

  constexpr tuple(const tuple&) = default;

  constexpr tuple(tuple&& o) = default;

  template <typename... OTs>
  TAOCPP_ANNOTATION constexpr tuple(const tuple<OTs...>& o)
    : Impl(static_cast<detail::tuple_impl<0, OTs...>&>(o))
  {
  }

  template <typename... OTs>
  TAOCPP_ANNOTATION constexpr tuple(tuple<OTs...>&& o)
    : Impl(static_cast<detail::tuple_impl<0, OTs...>&&>(o))
  {
  }

  TAOCPP_ANNOTATION
  tuple& operator=(const tuple& o)
  {
    this->Impl::operator=(o);
    return *this;
  }

  TAOCPP_ANNOTATION
  tuple& operator=(tuple&& o)
  {
    this->Impl::operator=(std::move(o));
    return *this;
  }

  template <typename... OTs>
  TAOCPP_ANNOTATION typename std::enable_if<sizeof...(Ts) == sizeof...(OTs), tuple&>::type
  operator=(const tuple<OTs...>& o)
  {
    this->Impl::operator=(o);
    return *this;
  }

  template <typename... OTs>
  TAOCPP_ANNOTATION typename std::enable_if<sizeof...(Ts) == sizeof...(OTs), tuple&>::type
  operator=(tuple<OTs...>&& o)
  {
    this->Impl::operator=(std::move(o));
    return *this;
  }
};

// Specialize for empty tuple:
template <>
class tuple<>
{
public:
  tuple() = default;
};

/// Reimplementation of std::tuple_size with markup for device support.
template <typename TupleType>
struct tuple_size;

template <typename... Ts>
struct tuple_size<tuple<Ts...>>
  : public std::integral_constant<std::size_t, static_cast<std::size_t>(sizeof...(Ts))>
{
  static const std::size_t value = static_cast<std::size_t>(sizeof...(Ts));
};

/// Reimplementation of std::tuple_element with markup for device support.
template <std::size_t Idx, typename TupleType>
struct tuple_element;

template <std::size_t Idx, typename Head, typename... Tail>
struct tuple_element<Idx, tuple<Head, Tail...>> : public tuple_element<Idx - 1, tuple<Tail...>>
{
};

template <typename Head, typename... Tail>
struct tuple_element<0, tuple<Head, Tail...>>
{
  using type = Head;
};

template <std::size_t Idx>
struct tuple_element<Idx, tuple<>>
{
  static_assert(Idx < tuple_size<tuple<>>::value, "Tuple index valid.");
};

/// Reimplementation of std::get with markup for device support.
template <std::size_t Idx, typename... Ts>
TAOCPP_ANNOTATION constexpr const typename tuple_element<Idx, tuple<Ts...>>::type& get(
  const tuple<Ts...>& t) noexcept
{
  return detail::get_helper<Idx>(t);
}

template <std::size_t Idx, typename... Ts>
TAOCPP_ANNOTATION constexpr typename tuple_element<Idx, tuple<Ts...>>::type& get(
  tuple<Ts...>& t) noexcept
{
  return detail::get_helper<Idx>(t);
}

template <std::size_t Idx, typename... Ts>
TAOCPP_ANNOTATION constexpr typename tuple_element<Idx, tuple<Ts...>>::type&& get(
  tuple<Ts...>&& t) noexcept
{
  using ResultType = typename tuple_element<Idx, tuple<Ts...>>::type;
  return std::forward<ResultType&&>(get<Idx>(t));
}

/// Reimplementation of std::make_tuple with markup for device support.
template <typename... Ts>
TAOCPP_ANNOTATION constexpr tuple<typename std::decay<Ts>::type...> make_tuple(Ts... ts)
{
  using ResultType = tuple<typename std::decay<Ts>::type...>;
  return ResultType(std::forward<Ts>(ts)...);
}

/// Reimplementation of std::tie with markup for device support.
template <typename... Ts>
TAOCPP_ANNOTATION constexpr tuple<Ts&...> tie(Ts&... ts) noexcept
{
  return tuple<Ts&...>(ts...);
}

/// Reimplementation of std::ignore with markup for device support.
static const detail::ignore_impl ignore{};

} // end namespace simple_tuple

#endif // SimpleTuple_h
