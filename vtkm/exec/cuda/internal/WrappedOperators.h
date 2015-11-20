//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_WrappedOperators_h
#define vtk_m_exec_cuda_internal_WrappedOperators_h

#include <vtkm/Types.h>
#include <vtkm/BinaryPredicates.h>
#include <vtkm/Pair.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/exec/cuda/internal/IteratorFromArrayPortal.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system/cuda/memory.h>
#include <boost/type_traits/remove_const.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace exec {
namespace cuda {
namespace internal {


// Unary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// PortalValue which happen when passed an input array that
// is implicit.
template<typename T_, typename Function>
struct  WrappedUnaryPredicate
{
  typedef typename boost::remove_const<T_>::type T;

  //make typedefs that thust expects unary operators to have
  typedef T first_argument_type;
  typedef bool result_type;

  Function m_f;

  VTKM_EXEC_EXPORT
  WrappedUnaryPredicate()
    : m_f()
  {}

  VTKM_CONT_EXPORT
  WrappedUnaryPredicate(const Function &f)
    : m_f(f)
  {}

  VTKM_EXEC_EXPORT bool operator()(const T &x) const
  {
    return m_f(x);
  }

  template<typename U>
  VTKM_EXEC_EXPORT bool operator()(const PortalValue<U> &x) const
  {
    return m_f((T)x);
  }

  VTKM_EXEC_EXPORT bool operator()(const thrust::system::cuda::pointer<T> x) const
  {
    return m_f(*x);
  }
};


// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// PortalValue which happen when passed an input array that
// is implicit.
template<typename T_, typename Function>
struct WrappedBinaryOperator
{
  typedef typename boost::remove_const<T_>::type T;

  //make typedefs that thust expects binary operators to have
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  Function m_f;

  VTKM_EXEC_EXPORT
  WrappedBinaryOperator()
    : m_f()
  {}

  VTKM_CONT_EXPORT
  WrappedBinaryOperator(const Function &f)
    : m_f(f)
  {}

  VTKM_EXEC_EXPORT T operator()(const T &x, const T &y) const
  {
    return m_f(x, y);
  }


  template<typename U>
  VTKM_EXEC_EXPORT T operator()(const T &x,
                                const PortalValue<U> &y) const
  {
    return m_f(x, (T)y);
  }

  template<typename U>
  VTKM_EXEC_EXPORT T operator()(const PortalValue<U> &x,
                                const T &y) const
  {
    return m_f((T)x, y);
  }

  template<typename U, typename V>
  VTKM_EXEC_EXPORT T operator()(const PortalValue<U> &x,
                                const PortalValue<V> &y) const
  {
    return m_f((T)x, (T)y);
  }

  VTKM_EXEC_EXPORT T operator()(const thrust::system::cuda::pointer<T> x,
                                const T* y) const
  {
    return m_f(*x, *y);
  }

  VTKM_EXEC_EXPORT T operator()(const thrust::system::cuda::pointer<T> x,
                                const T& y) const
  {
    return m_f(*x, y);
  }

  VTKM_EXEC_EXPORT T operator()(const T& x,
                                const thrust::system::cuda::pointer<T> y) const
  {
    return m_f(x, *y);
  }

  VTKM_EXEC_EXPORT T operator()(const thrust::system::cuda::pointer<T> x,
                                const thrust::system::cuda::pointer<T> y) const
  {
    return m_f(*x, *y);
  }

};

template<typename T_, typename Function>
struct WrappedBinaryPredicate
{
  typedef typename boost::remove_const<T_>::type T;

  //make typedefs that thust expects binary operators to have
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef bool result_type;

  Function m_f;

  VTKM_EXEC_EXPORT
  WrappedBinaryPredicate()
    : m_f()
  {}

  VTKM_CONT_EXPORT
  WrappedBinaryPredicate(const Function &f)
    : m_f(f)
  {}

  VTKM_EXEC_EXPORT bool operator()(const T &x, const T &y) const
  {
    return m_f(x, y);
  }


  template<typename U>
  VTKM_EXEC_EXPORT bool operator()(const T &x,
                                   const PortalValue<U> &y) const
  {
    return m_f(x, (T)y);
  }

  template<typename U>
  VTKM_EXEC_EXPORT bool operator()(const PortalValue<U> &x,
                                   const T &y) const
  {
    return m_f((T)x, y);
  }

  template<typename U, typename V>
  VTKM_EXEC_EXPORT bool operator()(const PortalValue<U> &x,
                                   const PortalValue<V> &y) const
  {
    return m_f((T)x, (T)y);
  }

  VTKM_EXEC_EXPORT bool operator()(const thrust::system::cuda::pointer<T> x,
                                   const T* y) const
  {
    return m_f(*x, *y);
  }

  VTKM_EXEC_EXPORT bool operator()(const thrust::system::cuda::pointer<T> x,
                                   const T& y) const
  {
    return m_f(*x, y);
  }

  VTKM_EXEC_EXPORT bool operator()(const T& x,
                                   const thrust::system::cuda::pointer<T> y) const
  {
    return m_f(x, *y);
  }

  VTKM_EXEC_EXPORT bool operator()(const thrust::system::cuda::pointer<T> x,
                                   const thrust::system::cuda::pointer<T> y) const
  {
    return m_f(*x, *y);
  }

};

}
}
}
} //namespace vtkm::exec::cuda::internal

namespace thrust { namespace detail {
//
// We tell Thrust that our WrappedBinaryOperator is commutative so that we
// activate numerous fast paths inside thrust which are only available when
// the binary functor is commutative and the T type is is_arithmetic
//
//
template< typename T, typename F>
struct is_commutative< vtkm::exec::cuda::internal::WrappedBinaryOperator<T, F> > :
      public thrust::detail::is_arithmetic<T> { };

} } //namespace thrust::detail

#endif //vtk_m_exec_cuda_internal_WrappedOperators_h
