//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_Variant_h
#define vtk_m_internal_Variant_h

#include <vtkm/internal/VariantDetail.h>

#include <vtkm/Deprecated.h>
#include <vtkm/List.h>

#if defined(VTKM_USING_GLIBCXX_4)
// It would make sense to put this in its own header file, but it is hard to imagine needing
// aligned_union anywhere else.
#include <algorithm>
namespace vtkmstd
{

template <std::size_t... Xs>
struct max_size;
template <std::size_t X>
struct max_size<X>
{
  static constexpr std::size_t value = X;
};
template <std::size_t X0, std::size_t... Xs>
struct max_size<X0, Xs...>
{
  static constexpr std::size_t other_value = max_size<Xs...>::value;
  static constexpr std::size_t value = (other_value > X0) ? other_value : X0;
};

// This is to get around an apparent bug in GCC 4.8 where alianas(x) does not
// seem to work when x is a constexpr. See
// https://stackoverflow.com/questions/29879609/g-complains-constexpr-function-is-not-a-constant-expression
template <std::size_t Alignment, std::size_t Size>
struct aligned_data_block
{
  alignas(Alignment) char _s[Size];
};

template <std::size_t Len, class... Types>
struct aligned_union
{
  static constexpr std::size_t alignment_value = vtkmstd::max_size<alignof(Types)...>::value;

  using type =
    vtkmstd::aligned_data_block<alignment_value, vtkmstd::max_size<Len, sizeof(Types)...>::value>;
};

// GCC 4.8 and 4.9 standard library does not support std::is_trivially_copyable.
// There is no relyable way to get this information (since it has to come special from
// the compiler). For our purposes, we will report as nothing being trivially copyable,
// which causes us to call the constructors with everything. This should be fine unless
// some other part of the compiler is trying to check for trivial copies (perhaps nvcc
// on top of GCC 4.8).
template <typename>
struct is_trivially_copyable : std::false_type
{
};

} // namespace vtkmstd

#else // NOT VTKM_USING_GLIBCXX_4
namespace vtkmstd
{

using std::aligned_union;
using std::is_trivially_copyable;

} // namespace vtkmstd
#endif

namespace vtkm
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
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VTKM_EXEC_CONT void operator()(const T& src, void* destPointer) const noexcept
  {
    new (destPointer) T(src);
  }
};

struct VariantDestroyFunctor
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VTKM_EXEC_CONT void operator()(T& src) const noexcept
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
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_copyable<T0>::value &&
                            vtkmstd::is_trivially_copyable<T1>::value &&
                            vtkmstd::is_trivially_copyable<T2>::value &&
                            vtkmstd::is_trivially_copyable<T3>::value)>
{
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename... Ts>
struct AllTriviallyCopyable<T0, T1, T2, T3, T4, Ts...>
  : std::integral_constant<bool,
                           (vtkmstd::is_trivially_copyable<T0>::value &&
                            vtkmstd::is_trivially_copyable<T1>::value &&
                            vtkmstd::is_trivially_copyable<T2>::value &&
                            vtkmstd::is_trivially_copyable<T3>::value &&
                            vtkmstd::is_trivially_copyable<T4>::value &&
                            AllTriviallyCopyable<Ts...>::value)>
{
};

template <typename VariantType>
struct VariantTriviallyCopyable;

template <typename... Ts>
struct VariantTriviallyCopyable<vtkm::internal::Variant<Ts...>> : AllTriviallyCopyable<Ts...>
{
};

template <typename... Ts>
struct VariantStorageImpl
{
  typename vtkmstd::aligned_union<0, Ts...>::type Storage;

  vtkm::IdComponent Index = -1;

  template <vtkm::IdComponent Index>
  using TypeAt = typename vtkm::ListAt<vtkm::List<Ts...>, Index>;

  VTKM_EXEC_CONT void* GetPointer() { return reinterpret_cast<void*>(&this->Storage); }
  VTKM_EXEC_CONT const void* GetPointer() const
  {
    return reinterpret_cast<const void*>(&this->Storage);
  }

  VTKM_EXEC_CONT vtkm::IdComponent GetIndex() const noexcept { return this->Index; }
  VTKM_EXEC_CONT bool IsValid() const noexcept { return this->GetIndex() >= 0; }

  VTKM_EXEC_CONT void Reset() noexcept
  {
    if (this->IsValid())
    {
      this->CastAndCall(detail::VariantDestroyFunctor{});
      this->Index = -1;
    }
  }

  template <typename Functor, typename... Args>
  VTKM_EXEC_CONT auto CastAndCall(Functor&& f, Args&&... args) const
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
  VTKM_EXEC_CONT auto CastAndCall(Functor&& f, Args&&... args) noexcept(
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
          typename TriviallyCopyable = typename VariantTriviallyCopyable<VariantType>::type>
struct VariantConstructorImpl;

template <typename... Ts>
struct VariantConstructorImpl<vtkm::internal::Variant<Ts...>, std::true_type>
  : VariantStorageImpl<Ts...>
{
  VariantConstructorImpl() = default;
  ~VariantConstructorImpl() = default;

  VariantConstructorImpl(const VariantConstructorImpl&) = default;
  VariantConstructorImpl(VariantConstructorImpl&&) = default;
  VariantConstructorImpl& operator=(const VariantConstructorImpl&) = default;
  VariantConstructorImpl& operator=(VariantConstructorImpl&&) = default;
};

template <typename... Ts>
struct VariantConstructorImpl<vtkm::internal::Variant<Ts...>, std::false_type>
  : VariantStorageImpl<Ts...>
{
  VariantConstructorImpl() = default;

  VTKM_EXEC_CONT ~VariantConstructorImpl() { this->Reset(); }

  VTKM_EXEC_CONT VariantConstructorImpl(const VariantConstructorImpl& src) noexcept
  {
    src.CastAndCall(VariantCopyFunctor{}, this->GetPointer());
    this->Index = src.Index;
  }

  VTKM_EXEC_CONT VariantConstructorImpl(VariantConstructorImpl&& rhs) noexcept
  {
    this->Storage = std::move(rhs.Storage);
    this->Index = std::move(rhs.Index);
    rhs.Index = -1;
  }

  VTKM_EXEC_CONT VariantConstructorImpl& operator=(const VariantConstructorImpl& src) noexcept
  {
    this->Reset();
    src.CastAndCall(detail::VariantCopyFunctor{}, this->GetPointer());
    this->Index = src.Index;
    return *this;
  }

  VTKM_EXEC_CONT VariantConstructorImpl& operator=(VariantConstructorImpl&& rhs) noexcept
  {
    this->Reset();
    this->Storage = std::move(rhs.Storage);
    this->Index = std::move(rhs.Index);
    rhs.Index = -1;
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
  /// stored (i.e. the Variant is invalid), -1 is returned.
  ///
  VTKM_EXEC_CONT vtkm::IdComponent GetIndex() const noexcept
  {
    return this->Superclass::GetIndex();
  }

  /// Returns true if this Variant is storing an object from one of the types in the template
  /// list, false otherwise.
  ///
  VTKM_EXEC_CONT bool IsValid() const noexcept { return this->Superclass::IsValid(); }

  /// Type that converts to a std::integral_constant containing the index of the given type (or
  /// -1 if that type is not in the list).
  template <typename T>
  using IndexOf = vtkm::ListIndexOf<vtkm::List<Ts...>, T>;

  /// Returns the index for the given type (or -1 if that type is not in the list).
  ///
  template <typename T>
  VTKM_EXEC_CONT static constexpr vtkm::IdComponent GetIndexOf()
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
  VTKM_EXEC_CONT Variant(const T& src) noexcept
  {
    constexpr vtkm::IdComponent index = GetIndexOf<T>();
    // Might be a way to use an enable_if to enforce a proper type.
    VTKM_STATIC_ASSERT_MSG(index >= 0, "Attempting to put invalid type into a Variant");

    new (this->GetPointer()) T(src);
    this->Index = index;
  }

  template <typename T>
  VTKM_EXEC_CONT Variant(const T&& src) noexcept
  {
    constexpr vtkm::IdComponent index = IndexOf<T>::value;
    // Might be a way to use an enable_if to enforce a proper type.
    VTKM_STATIC_ASSERT_MSG(index >= 0, "Attempting to put invalid type into a Variant");

    new (this->GetPointer()) T(std::move(src));
    this->Index = index;
  }

  template <typename T, typename... Args>
  VTKM_EXEC_CONT T& Emplace(Args&&... args)
  {
    constexpr vtkm::IdComponent I = GetIndexOf<T>();
    VTKM_STATIC_ASSERT_MSG(I >= 0, "Variant::Emplace called with invalid type.");
    return this->EmplaceImpl<T, I>(std::forward<Args>(args)...);
  }

  template <typename T, typename U, typename... Args>
  VTKM_EXEC_CONT T& Emplace(std::initializer_list<U> il, Args&&... args)
  {
    constexpr vtkm::IdComponent I = GetIndexOf<T>();
    VTKM_STATIC_ASSERT_MSG(I >= 0, "Variant::Emplace called with invalid type.");
    return this->EmplaceImpl<T, I>(il, std::forward<Args>(args)...);
  }

  template <vtkm::IdComponent I, typename... Args>
  VTKM_EXEC_CONT TypeAt<I>& Emplace(Args&&... args)
  {
    VTKM_STATIC_ASSERT_MSG((I >= 0) && (I < NumberOfTypes),
                           "Variant::Emplace called with invalid index");
    return this->EmplaceImpl<TypeAt<I>, I>(std::forward<Args>(args)...);
  }

  template <vtkm::IdComponent I, typename U, typename... Args>
  VTKM_EXEC_CONT TypeAt<I>& Emplace(std::initializer_list<U> il, Args&&... args)
  {
    VTKM_STATIC_ASSERT_MSG((I >= 0) && (I < NumberOfTypes),
                           "Variant::Emplace called with invalid index");
    return this->EmplaceImpl<TypeAt<I>, I>(il, std::forward<Args>(args)...);
  }

private:
  template <typename T, vtkm::IdComponent I, typename... Args>
  VTKM_EXEC_CONT T& EmplaceImpl(Args&&... args)
  {
    this->Reset();
    T* value = new (this->GetPointer()) T{ args... };
    this->Index = I;
    return *value;
  }

  template <typename T, vtkm::IdComponent I, typename U, typename... Args>
  VTKM_EXEC_CONT T& EmplaceImpl(std::initializer_list<U> il, Args&&... args)
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
  VTKM_EXEC_CONT TypeAt<I>& Get() noexcept
  {
    VTKM_ASSERT(I == this->GetIndex());
    return *reinterpret_cast<TypeAt<I>*>(this->GetPointer());
  }

  template <vtkm::IdComponent I>
  VTKM_EXEC_CONT const TypeAt<I>& Get() const noexcept
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
  VTKM_EXEC_CONT T& Get() noexcept
  {
    VTKM_ASSERT(this->GetIndexOf<T>() == this->GetIndex());
    return *reinterpret_cast<T*>(this->GetPointer());
  }

  template <typename T>
  VTKM_EXEC_CONT const T& Get() const noexcept
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
  VTKM_EXEC_CONT auto CastAndCall(Functor&& f, Args&&... args) const
    noexcept(noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
      -> decltype(f(std::declval<const TypeAt<0>&>(), args...))
  {
    return this->Superclass::CastAndCall(std::forward<Functor>(f), std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  VTKM_EXEC_CONT auto CastAndCall(Functor&& f, Args&&... args) noexcept(
    noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
    -> decltype(f(std::declval<TypeAt<0>&>(), args...))
  {
    return this->Superclass::CastAndCall(std::forward<Functor>(f), std::forward<Args>(args)...);
  }

  /// Destroys any object the Variant is holding and sets the Variant to an invalid state. This
  /// method is not thread safe.
  ///
  VTKM_EXEC_CONT void Reset() noexcept { this->Superclass::Reset(); }
};

/// \brief Convert a ListTag to a Variant.
///
/// Depricated. Use ListAsVariant instead.
///
template <typename ListTag>
using ListTagAsVariant VTKM_DEPRECATED(
  1.6,
  "vtkm::ListTag is no longer supported. Use vtkm::List instead.") =
  vtkm::ListApply<ListTag, vtkm::internal::Variant>;

/// \brief Convert a `List` to a `Variant`.
///
template <typename List>
using ListAsVariant = vtkm::ListApply<List, vtkm::internal::Variant>;
}
} // namespace vtkm::internal

#endif //vtk_m_internal_Variant_h
