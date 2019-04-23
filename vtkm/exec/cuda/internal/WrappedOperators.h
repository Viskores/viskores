//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_WrappedOperators_h
#define vtk_m_exec_cuda_internal_WrappedOperators_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/exec/cuda/internal/IteratorFromArrayPortal.h>
#include <vtkm/internal/ExportMacros.h>

// Disable warnings we check vtkm for but Thrust does not.
#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system/cuda/memory.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

// Unary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename T_, typename Function>
struct WrappedUnaryPredicate
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects unary operators to have
  using first_argument_type = T;
  using result_type = bool;

  Function m_f;

  VTKM_EXEC
  WrappedUnaryPredicate()
    : m_f()
  {
  }

  VTKM_CONT
  WrappedUnaryPredicate(const Function& f)
    : m_f(f)
  {
  }

  VTKM_EXEC bool operator()(const T& x) const { return m_f(x); }

  template <typename U>
  VTKM_EXEC bool operator()(const vtkm::internal::ArrayPortalValueReference<U>& x) const
  {
    return m_f(x.Get());
  }

  VTKM_EXEC bool operator()(const T* x) const { return m_f(*x); }
};

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename T_, typename Function>
struct WrappedBinaryOperator
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects binary operators to have
  using first_argument_type = T;
  using second_argument_type = T;
  using result_type = T;

  Function m_f;

  VTKM_EXEC
  WrappedBinaryOperator()
    : m_f()
  {
  }

  VTKM_CONT
  WrappedBinaryOperator(const Function& f)
    : m_f(f)
  {
  }

  VTKM_EXEC T operator()(const T& x, const T& y) const { return m_f(x, y); }

  template <typename U>
  VTKM_EXEC T operator()(const T& x, const vtkm::internal::ArrayPortalValueReference<U>& y) const
  {
    // to support proper implicit conversion, and avoid overload
    // ambiguities.
    return m_f(x, y.Get());
  }

  template <typename U>
  VTKM_EXEC T operator()(const vtkm::internal::ArrayPortalValueReference<U>& x, const T& y) const
  {
    return m_f(x.Get(), y);
  }

  template <typename U, typename V>
  VTKM_EXEC T operator()(const vtkm::internal::ArrayPortalValueReference<U>& x,
                         const vtkm::internal::ArrayPortalValueReference<V>& y) const
  {
    return m_f(x.Get(), y.Get());
  }

  VTKM_EXEC T operator()(const T* const x, const T& y) const { return m_f(*x, y); }

  VTKM_EXEC T operator()(const T& x, const T* const y) const { return m_f(x, *y); }

  VTKM_EXEC T operator()(const T* const x, const T* const y) const { return m_f(*x, *y); }
};

template <typename T_, typename Function>
struct WrappedBinaryPredicate
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects binary operators to have
  using first_argument_type = T;
  using second_argument_type = T;
  using result_type = bool;

  Function m_f;

  VTKM_EXEC
  WrappedBinaryPredicate()
    : m_f()
  {
  }

  VTKM_CONT
  WrappedBinaryPredicate(const Function& f)
    : m_f(f)
  {
  }

  VTKM_EXEC bool operator()(const T& x, const T& y) const { return m_f(x, y); }

  template <typename U>
  VTKM_EXEC bool operator()(const T& x, const vtkm::internal::ArrayPortalValueReference<U>& y) const
  {
    return m_f(x, y.Get());
  }

  template <typename U>
  VTKM_EXEC bool operator()(const vtkm::internal::ArrayPortalValueReference<U>& x, const T& y) const
  {
    return m_f(x.Get(), y);
  }

  template <typename U, typename V>
  VTKM_EXEC bool operator()(const vtkm::internal::ArrayPortalValueReference<U>& x,
                            const vtkm::internal::ArrayPortalValueReference<V>& y) const
  {
    return m_f(x.Get(), y.Get());
  }

  VTKM_EXEC bool operator()(const T* const x, const T& y) const { return m_f(*x, y); }

  VTKM_EXEC bool operator()(const T& x, const T* const y) const { return m_f(x, *y); }

  VTKM_EXEC bool operator()(const T* const x, const T* const y) const { return m_f(*x, *y); }
};
}
}
}
} //namespace vtkm::exec::cuda::internal

namespace thrust
{
namespace detail
{
//
// We tell Thrust that our WrappedBinaryOperator is commutative so that we
// activate numerous fast paths inside thrust which are only available when
// the binary functor is commutative and the T type is is_arithmetic
//
//
template <typename T, typename F>
struct is_commutative<vtkm::exec::cuda::internal::WrappedBinaryOperator<T, F>>
  : public thrust::detail::is_arithmetic<T>
{
};
}
} //namespace thrust::detail

#endif //vtk_m_exec_cuda_internal_WrappedOperators_h
