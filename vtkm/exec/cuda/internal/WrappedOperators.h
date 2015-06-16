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
#include <vtkm/Pair.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/exec/cuda/internal/IteratorFromArrayPortal.h>

// Disable warnings we check vtkm for but Thrust does not.
#if defined(__GNUC__) || defined(____clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

#include <thrust/system/cuda/memory.h>

#if defined(__GNUC__) || defined(____clang__)
#pragma GCC diagnostic pop
#endif // gcc || clang

namespace vtkm {
namespace exec {
namespace cuda {
namespace internal {

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// PortalValue which happen when passed an input array that
// is implicit.
template<typename ResultType, typename Function>
  struct WrappedBinaryOperator
{
  Function m_f;

  VTKM_EXEC_EXPORT
  WrappedBinaryOperator()
    : m_f()
  {}

  VTKM_CONT_EXPORT
  WrappedBinaryOperator(const Function &f)
    : m_f(f)
  {}

  template<typename T, typename U>
  VTKM_EXEC_EXPORT ResultType operator()(const T &x, const U &y) const
  {
    return m_f(x, y);
  }

  template<typename T, typename U>
  VTKM_EXEC_EXPORT ResultType operator()(const T &x,
                                         const PortalValue<U> &y) const
  {
    typedef typename PortalValue<U>::ValueType ValueType;
    return m_f(x, (ValueType)y);
  }

  template<typename T, typename U>
  VTKM_EXEC_EXPORT ResultType operator()(const PortalValue<T> &x,
                                         const U &y) const
  {
    typedef typename PortalValue<T>::ValueType ValueType;
    return m_f((ValueType)x, y);
  }

  template<typename T, typename U>
  VTKM_EXEC_EXPORT ResultType operator()(const PortalValue<T> &x,
                                         const PortalValue<U> &y) const
  {
    typedef typename PortalValue<T>::ValueType ValueTypeT;
    typedef typename PortalValue<U>::ValueType ValueTypeU;
    return m_f((ValueTypeT)x, (ValueTypeU)y);
  }

  template<typename T>
  VTKM_EXEC_EXPORT ResultType operator()(const thrust::system::cuda::pointer<T> x,
                                         const T* y) const
  {
    return m_f(*x, *y);
  }

  template<typename T>
  VTKM_EXEC_EXPORT ResultType operator()(const thrust::system::cuda::pointer<T> x,
                                         const T& y) const
  {
    return m_f(*x, y);
  }

  template<typename T>
  VTKM_EXEC_EXPORT ResultType operator()(const T& x,
                                         const thrust::system::cuda::pointer<T> y) const
  {
    return m_f(x, *y);
  }

  template<typename T>
  VTKM_EXEC_EXPORT ResultType operator()(const thrust::system::cuda::pointer<T> x,
                                         const thrust::system::cuda::pointer<T> y) const
  {
    return m_f(*x, *y);
  }

};

}
}
}
} //namespace vtkm::exec::cuda::internal


#endif //vtk_m_exec_cuda_internal_WrappedOperators_h