//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#if !defined(VTK_M_DEVICE) || !defined(VTK_M_NAMESPACE)
#error VariantImpl.h must be included from Variant.h
// Some defines to make my IDE happy.
#define VTK_M_DEVICE
#define VTK_M_NAMESPACE tmp
#endif

#include <vtkm/internal/VariantImplDetail.h>

#include <vtkm/Deprecated.h>
#include <vtkm/List.h>

#include <vtkmstd/aligned_union.h>
#include <vtkmstd/is_trivial.h>

namespace vtkm
{
namespace VTK_M_NAMESPACE
{
namespace internal
{

// Forward declaration
template <typename... Ts>
class Variant;

namespace detail
{

struct VariantCopyFunctor
{
  template <typename T>
  VTK_M_DEVICE void operator()(const T& src, void* destPointer) const noexcept
  {
    new (destPointer) T(src);
  }
};

struct VariantDestroyFunctor
{
  template <typename T>
  VTK_M_DEVICE void operator()(T& src) const noexcept
  {
    src.~T();
  }
};

template <typename... Ts>
struct AllTriviallyCopyable;

template <>
struct AllTriviallyCopyable<> : std::true_type
{
};

template <typename T0>
struct AllTriviallyCopyable<T0>
  : std::integral_constant<bool, (vtkmstd::is_trivially_copyable<T0>::value)>
{
};

template <typename T0, typename T1>
struct AllTriviallyCopyable<T0, T1>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_copyable<T0>::value &&
                            vtkmstd::is_trivially_copyable<T1>::value)>
{
};

template <typename T0, typename T1, typename T2>
struct AllTriviallyCopyable<T0, T1, T2>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_copyable<T0>::value &&
                            vtkmstd::is_trivially_copyable<T1>::value &&
                            vtkmstd::is_trivially_copyable<T2>::value)>
{
};

template <typename T0, typename T1, typename T2, typename T3>
struct AllTriviallyCopyable<T0, T1, T2, T3>
  : std::integral_constant<
      bool,
      (vtkmstd::is_trivially_copyable<T0>::value && vtkmstd::is_trivially_copyable<T1>::value &&
       vtkmstd::is_trivially_copyable<T2>::value && vtkmstd::is_trivially_copyable<T3>::value)>
{
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename... Ts>
struct AllTriviallyCopyable<T0, T1, T2, T3, T4, Ts...>
  : std::integral_constant<
      bool,
      (vtkmstd::is_trivially_copyable<T0>::value && vtkmstd::is_trivially_copyable<T1>::value &&
       vtkmstd::is_trivially_copyable<T2>::value && vtkmstd::is_trivially_copyable<T3>::value &&
       vtkmstd::is_trivially_copyable<T4>::value && AllTriviallyCopyable<Ts...>::value)>
{
};

template <typename VariantType>
struct VariantTriviallyCopyable;

template <typename... Ts>
struct VariantTriviallyCopyable<vtkm::VTK_M_NAMESPACE::internal::Variant<Ts...>>
  : AllTriviallyCopyable<Ts...>
{
};

template <typename... Ts>
struct AllTriviallyConstructible;

template <>
struct AllTriviallyConstructible<> : std::true_type
{
};

template <typename T0>
struct AllTriviallyConstructible<T0>
  : std::integral_constant<bool, (vtkmstd::is_trivially_constructible<T0>::value)>
{
};

template <typename T0, typename T1>
struct AllTriviallyConstructible<T0, T1>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_constructible<T0>::value &&
                            vtkmstd::is_trivially_constructible<T1>::value)>
{
};

template <typename T0, typename T1, typename T2>
struct AllTriviallyConstructible<T0, T1, T2>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_constructible<T0>::value &&
                            vtkmstd::is_trivially_constructible<T1>::value &&
                            vtkmstd::is_trivially_constructible<T2>::value)>
{
};

template <typename T0, typename T1, typename T2, typename T3>
struct AllTriviallyConstructible<T0, T1, T2, T3>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_constructible<T0>::value &&
                            vtkmstd::is_trivially_constructible<T1>::value &&
                            vtkmstd::is_trivially_constructible<T2>::value &&
                            vtkmstd::is_trivially_constructible<T3>::value)>
{
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename... Ts>
struct AllTriviallyConstructible<T0, T1, T2, T3, T4, Ts...>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_constructible<T0>::value &&
                            vtkmstd::is_trivially_constructible<T1>::value &&
                            vtkmstd::is_trivially_constructible<T2>::value &&
                            vtkmstd::is_trivially_constructible<T3>::value &&
                            vtkmstd::is_trivially_constructible<T4>::value &&
                            AllTriviallyConstructible<Ts...>::value)>
{
};

template <typename VariantType>
struct VariantTriviallyConstructible;

template <typename... Ts>
struct VariantTriviallyConstructible<vtkm::VTK_M_NAMESPACE::internal::Variant<Ts...>>
  : AllTriviallyConstructible<Ts...>
{
};

template <typename... Ts>
struct VariantStorageImpl
{
  typename vtkmstd::aligned_union<0, Ts...>::type Storage;

  vtkm::IdComponent Index;

  template <vtkm::IdComponent Index>
  using TypeAt = typename vtkm::ListAt<vtkm::List<Ts...>, Index>;

  VTK_M_DEVICE void* GetPointer() { return reinterpret_cast<void*>(&this->Storage); }
  VTK_M_DEVICE const void* GetPointer() const
  {
    return reinterpret_cast<const void*>(&this->Storage);
  }

  VTK_M_DEVICE vtkm::IdComponent GetIndex() const noexcept { return this->Index; }
  VTK_M_DEVICE bool IsValid() const noexcept
  {
    return (this->Index >= 0) && (this->Index < static_cast<vtkm::IdComponent>(sizeof...(Ts)));
  }

  VTK_M_DEVICE void Reset() noexcept
  {
    if (this->IsValid())
    {
      this->CastAndCall(detail::VariantDestroyFunctor{});
      this->Index = -1;
    }
  }

  template <typename Functor, typename... Args>
  VTK_M_DEVICE auto CastAndCall(Functor&& f, Args&&... args) const
    noexcept(noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
      -> decltype(f(std::declval<const TypeAt<0>&>(), args...))
  {
    VTKM_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<decltype(f(std::declval<const TypeAt<0>&>(), args...))>(
      brigand::list<Ts...>{},
      this->GetIndex(),
      std::forward<Functor>(f),
      this->GetPointer(),
      std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  VTK_M_DEVICE auto CastAndCall(Functor&& f, Args&&... args) noexcept(
    noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
    -> decltype(f(std::declval<TypeAt<0>&>(), args...))
  {
    VTKM_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<decltype(f(std::declval<TypeAt<0>&>(), args...))>(
      brigand::list<Ts...>{},
      this->GetIndex(),
      std::forward<Functor>(f),
      this->GetPointer(),
      std::forward<Args>(args)...);
  }
};

template <typename VariantType,
          typename TriviallyConstructible =
            typename VariantTriviallyConstructible<VariantType>::type,
          typename TriviallyCopyable = typename VariantTriviallyCopyable<VariantType>::type>
struct VariantConstructorImpl;

// Can trivially construct, deconstruct, and copy all data. (Probably all trivial classes.)
template <typename... Ts>
struct VariantConstructorImpl<vtkm::VTK_M_NAMESPACE::internal::Variant<Ts...>,
                              std::true_type,
                              std::true_type> : VariantStorageImpl<Ts...>
{
  VariantConstructorImpl() = default;
  ~VariantConstructorImpl() = default;

  VariantConstructorImpl(const VariantConstructorImpl&) = default;
  VariantConstructorImpl(VariantConstructorImpl&&) = default;
  VariantConstructorImpl& operator=(const VariantConstructorImpl&) = default;
  VariantConstructorImpl& operator=(VariantConstructorImpl&&) = default;
};

// Can trivially copy, but cannot trivially construct. Common if a class is simple but
// initializes itself.
template <typename... Ts>
struct VariantConstructorImpl<vtkm::VTK_M_NAMESPACE::internal::Variant<Ts...>,
                              std::false_type,
                              std::true_type> : VariantStorageImpl<Ts...>
{
  VTK_M_DEVICE VariantConstructorImpl() { this->Index = -1; }

  // Any trivially copyable class is trivially destructable.
  ~VariantConstructorImpl() = default;

  VariantConstructorImpl(const VariantConstructorImpl&) = default;
  VariantConstructorImpl(VariantConstructorImpl&&) = default;
  VariantConstructorImpl& operator=(const VariantConstructorImpl&) = default;
  VariantConstructorImpl& operator=(VariantConstructorImpl&&) = default;
};

// Cannot trivially copy. We assume we cannot trivially construct/destruct.
template <typename construct_type, typename... Ts>
struct VariantConstructorImpl<vtkm::VTK_M_NAMESPACE::internal::Variant<Ts...>,
                              construct_type,
                              std::false_type> : VariantStorageImpl<Ts...>
{
  VTK_M_DEVICE VariantConstructorImpl() { this->Index = -1; }
  VTK_M_DEVICE ~VariantConstructorImpl() { this->Reset(); }

  VTK_M_DEVICE VariantConstructorImpl(const VariantConstructorImpl& src) noexcept
  {
    src.CastAndCall(VariantCopyFunctor{}, this->GetPointer());
    this->Index = src.Index;
  }

  VTK_M_DEVICE VariantConstructorImpl& operator=(const VariantConstructorImpl& src) noexcept
  {
    this->Reset();
    src.CastAndCall(detail::VariantCopyFunctor{}, this->GetPointer());
    this->Index = src.Index;
    return *this;
  }
};

} // namespace detail

template <typename... Ts>
class Variant : detail::VariantConstructorImpl<Variant<Ts...>>
{
  using Superclass = detail::VariantConstructorImpl<Variant<Ts...>>;

public:
  /// Returns the index of the type of object this variant is storing. If no object is currently
  /// stored (i.e. the `Variant` is invalid), an invalid is returned.
  ///
  VTK_M_DEVICE vtkm::IdComponent GetIndex() const noexcept { return this->Superclass::GetIndex(); }

  /// Returns true if this `Variant` is storing an object from one of the types in the template
  /// list, false otherwise.
  ///
  /// Note that if this `Variant` was not initialized with an object, the result of `IsValid`
  /// is undefined. The `Variant` could report itself as validly containing an object that
  /// is trivially constructed.
  ///
  VTK_M_DEVICE bool IsValid() const noexcept { return this->Superclass::IsValid(); }

  /// Type that converts to a std::integral_constant containing the index of the given type (or
  /// -1 if that type is not in the list).
  template <typename T>
  using IndexOf = vtkm::ListIndexOf<vtkm::List<Ts...>, T>;

  /// Returns the index for the given type (or -1 if that type is not in the list).
  ///
  template <typename T>
  VTK_M_DEVICE static constexpr vtkm::IdComponent GetIndexOf()
  {
    return IndexOf<T>::value;
  }

  /// Type that converts to the type at the given index.
  ///
  template <vtkm::IdComponent Index>
  using TypeAt = typename Superclass::template TypeAt<Index>;

  /// The number of types representable by this Variant.
  ///
  static constexpr vtkm::IdComponent NumberOfTypes = vtkm::IdComponent{ sizeof...(Ts) };

  Variant() = default;
  ~Variant() = default;
  Variant(const Variant&) = default;
  Variant(Variant&&) = default;
  Variant& operator=(const Variant&) = default;
  Variant& operator=(Variant&&) = default;

  template <typename T>
  VTK_M_DEVICE Variant(const T& src) noexcept
  {
    constexpr vtkm::IdComponent index = GetIndexOf<T>();
    // Might be a way to use an enable_if to enforce a proper type.
    VTKM_STATIC_ASSERT_MSG(index >= 0, "Attempting to put invalid type into a Variant");

    new (this->GetPointer()) T(src);
    this->Index = index;
  }

  template <typename T>
  VTK_M_DEVICE Variant(const T&& src) noexcept
  {
    constexpr vtkm::IdComponent index = IndexOf<T>::value;
    // Might be a way to use an enable_if to enforce a proper type.
    VTKM_STATIC_ASSERT_MSG(index >= 0, "Attempting to put invalid type into a Variant");

    new (this->GetPointer()) T(std::move(src));
    this->Index = index;
  }

  template <typename T, typename... Args>
  VTK_M_DEVICE T& Emplace(Args&&... args)
  {
    constexpr vtkm::IdComponent I = GetIndexOf<T>();
    VTKM_STATIC_ASSERT_MSG(I >= 0, "Variant::Emplace called with invalid type.");
    return this->EmplaceImpl<T, I>(std::forward<Args>(args)...);
  }

  template <typename T, typename U, typename... Args>
  VTK_M_DEVICE T& Emplace(std::initializer_list<U> il, Args&&... args)
  {
    constexpr vtkm::IdComponent I = GetIndexOf<T>();
    VTKM_STATIC_ASSERT_MSG(I >= 0, "Variant::Emplace called with invalid type.");
    return this->EmplaceImpl<T, I>(il, std::forward<Args>(args)...);
  }

  template <vtkm::IdComponent I, typename... Args>
  VTK_M_DEVICE TypeAt<I>& Emplace(Args&&... args)
  {
    VTKM_STATIC_ASSERT_MSG((I >= 0) && (I < NumberOfTypes),
                           "Variant::Emplace called with invalid index");
    return this->EmplaceImpl<TypeAt<I>, I>(std::forward<Args>(args)...);
  }

  template <vtkm::IdComponent I, typename U, typename... Args>
  VTK_M_DEVICE TypeAt<I>& Emplace(std::initializer_list<U> il, Args&&... args)
  {
    VTKM_STATIC_ASSERT_MSG((I >= 0) && (I < NumberOfTypes),
                           "Variant::Emplace called with invalid index");
    return this->EmplaceImpl<TypeAt<I>, I>(il, std::forward<Args>(args)...);
  }

private:
  template <typename T, vtkm::IdComponent I, typename... Args>
  VTK_M_DEVICE T& EmplaceImpl(Args&&... args)
  {
    this->Reset();
    T* value = new (this->GetPointer()) T{ args... };
    this->Index = I;
    return *value;
  }

  template <typename T, vtkm::IdComponent I, typename U, typename... Args>
  VTK_M_DEVICE T& EmplaceImpl(std::initializer_list<U> il, Args&&... args)
  {
    this->Reset();
    T* value = new (this->GetPointer()) T(il, args...);
    this->Index = I;
    return *value;
  }

public:
  //@{
  /// Returns the value as the type at the given index. The behavior is undefined if the
  /// variant does not contain the value at the given index.
  ///
  template <vtkm::IdComponent I>
  VTK_M_DEVICE TypeAt<I>& Get() noexcept
  {
    VTKM_ASSERT(I == this->GetIndex());
    return *reinterpret_cast<TypeAt<I>*>(this->GetPointer());
  }

  template <vtkm::IdComponent I>
  VTK_M_DEVICE const TypeAt<I>& Get() const noexcept
  {
    VTKM_ASSERT(I == this->GetIndex());
    return *reinterpret_cast<const TypeAt<I>*>(this->GetPointer());
  }
  //@}

  //@{
  /// Returns the value as the given type. The behavior is undefined if the variant does not
  /// contain a value of the given type.
  ///
  template <typename T>
  VTK_M_DEVICE T& Get() noexcept
  {
    VTKM_ASSERT(this->GetIndexOf<T>() == this->GetIndex());
    return *reinterpret_cast<T*>(this->GetPointer());
  }

  template <typename T>
  VTK_M_DEVICE const T& Get() const noexcept
  {
    VTKM_ASSERT(this->GetIndexOf<T>() == this->GetIndex());
    return *reinterpret_cast<const T*>(this->GetPointer());
  }
  //@}

  //@{
  /// Given a functor object, calls the functor with the contained object cast to the appropriate
  /// type. If extra \c args are given, then those are also passed to the functor after the cast
  /// object. If the functor returns a value, that value is returned from \c CastAndCall.
  ///
  /// The results are undefined if the Variant is not valid.
  ///
  template <typename Functor, typename... Args>
  VTK_M_DEVICE auto CastAndCall(Functor&& f, Args&&... args) const
    noexcept(noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
      -> decltype(f(std::declval<const TypeAt<0>&>(), args...))
  {
    return this->Superclass::CastAndCall(std::forward<Functor>(f), std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  VTK_M_DEVICE auto CastAndCall(Functor&& f, Args&&... args) noexcept(
    noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
    -> decltype(f(std::declval<TypeAt<0>&>(), args...))
  {
    return this->Superclass::CastAndCall(std::forward<Functor>(f), std::forward<Args>(args)...);
  }

  /// Destroys any object the Variant is holding and sets the Variant to an invalid state. This
  /// method is not thread safe.
  ///
  VTK_M_DEVICE void Reset() noexcept { this->Superclass::Reset(); }
};

/// \brief Convert a ListTag to a Variant.
///
/// Depricated. Use ListAsVariant instead.
///
template <typename ListTag>
using ListTagAsVariant VTKM_DEPRECATED(
  1.6,
  "vtkm::ListTag is no longer supported. Use vtkm::List instead.") =
  vtkm::ListApply<ListTag, vtkm::VTK_M_NAMESPACE::internal::Variant>;

/// \brief Convert a `List` to a `Variant`.
///
template <typename List>
using ListAsVariant = vtkm::ListApply<List, vtkm::VTK_M_NAMESPACE::internal::Variant>;
}
}
} // namespace vtkm::VTK_M_NAMESPACE::internal

#undef VTK_M_DEVICE
#undef VTK_M_NAMESPACE
