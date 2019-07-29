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

#include <vtkm/ListTag.h>


// It would make sense to put this in its own header file, but it is hard to imagine needing
// aligned_union anywhere else.
#if (defined(VTKM_GCC) && (__GNUC__ == 4)) || (defined(VTKM_ICC) && (__INTEL_COMPILER < 1800))
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
// This implementation comes from https://en.cppreference.com/w/cpp/types/aligned_union
template <std::size_t Len, class... Types>
struct aligned_union
{
  static constexpr std::size_t alignment_value = vtkmstd::max_size<alignof(Types)...>::value;

  struct type
  {
    alignas(alignment_value) char _s[vtkmstd::max_size<Len, sizeof(Types)...>::value];
  };
};
} // namespace vtkmstd
#else
namespace vtkmstd
{
using std::aligned_union;
} // namespace vtkmstd
#endif

namespace vtkm
{
namespace internal
{

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

} // namespace detail

template <typename... Ts>
class Variant
{
  struct ListTag : vtkm::ListTagBase<Ts...>
  {
  };

  typename vtkmstd::aligned_union<0, Ts...>::type Storage;

  VTKM_EXEC_CONT void* GetPointer() { return reinterpret_cast<void*>(&this->Storage); }
  VTKM_EXEC_CONT const void* GetPointer() const
  {
    return reinterpret_cast<const void*>(&this->Storage);
  }

  vtkm::IdComponent Index = -1;

public:
  /// Returns the index of the type of object this variant is storing. If no object is currently
  /// stored (i.e. the Variant is invalid), -1 is returned.
  ///
  VTKM_EXEC_CONT vtkm::IdComponent GetIndex() const noexcept { return this->Index; }

  /// Returns true if this Variant is storing an object from one of the types in the template
  /// list, false otherwise.
  ///
  VTKM_EXEC_CONT bool IsValid() const noexcept { return this->GetIndex() >= 0; }

  /// Type that converts to a std::integral_constant containing the index of the given type (or
  /// -1 if that type is not in the list).
  template <typename T>
  using IndexOf = std::integral_constant<vtkm::IdComponent, vtkm::ListIndexOf<ListTag, T>::value>;

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
  using TypeAt = typename vtkm::ListTypeAt<ListTag, Index>::type;

  /// The number of types representable by this Variant.
  ///
  static constexpr vtkm::IdComponent NumberOfTypes = vtkm::IdComponent{ sizeof...(Ts) };

  Variant() = default;

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

  VTKM_EXEC_CONT Variant(Variant&& rhs) noexcept
  {
    this->Storage = std::move(rhs.Storage);
    this->Index = std::move(rhs.Index);
    rhs.Index = -1;
  }

  VTKM_EXEC_CONT Variant(const Variant& src) noexcept
  {
    src.CastAndCall(detail::VariantCopyFunctor{}, this->GetPointer());
    this->Index = src.GetIndex();
  }

  VTKM_EXEC_CONT ~Variant() { this->Reset(); }

  VTKM_EXEC_CONT Variant& operator=(Variant&& rhs) noexcept
  {
    this->Reset();
    this->Storage = std::move(rhs.Storage);
    this->Index = std::move(rhs.Index);
    rhs.Index = -1;
    return *this;
  }

  VTKM_EXEC_CONT Variant& operator=(const Variant& src) noexcept
  {
    this->Reset();
    src.CastAndCall(detail::VariantCopyFunctor{}, this->GetPointer());
    this->Index = src.GetIndex();
    return *this;
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
    VTKM_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<decltype(f(std::declval<const TypeAt<0>&>(), args...))>(
      vtkm::internal::ListTagAsBrigandList<ListTag>(),
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
      vtkm::internal::ListTagAsBrigandList<ListTag>(),
      this->GetIndex(),
      std::forward<Functor>(f),
      this->GetPointer(),
      std::forward<Args>(args)...);
  }

  /// Destroys any object the Variant is holding and sets the Variant to an invalid state. This
  /// method is not thread safe.
  ///
  VTKM_EXEC_CONT void Reset() noexcept
  {
    if (this->IsValid())
    {
      this->CastAndCall(detail::VariantDestroyFunctor{});
      this->Index = -1;
    }
  }
};

/// \brief Convert a ListTag to a Variant.
///
template <typename ListTag>
using ListTagAsVariant = typename vtkm::ListTagApply<ListTag, vtkm::internal::Variant>::type;
}
} // namespace vtkm::internal

#endif //vtk_m_internal_Variant_h
