//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_ThrustPatches_h
#define vtk_m_exec_cuda_internal_ThrustPatches_h

#include <vtkm/Types.h>

#ifdef VTKM_ENABLE_CUDA

//So for thrust 1.8.0 - 1.8.2 the inclusive_scan has a bug when accumulating
//values when the binary operators states it is not commutative.
//For more complex value types, we patch thrust/bulk with fix that is found
//in issue: https://github.com/thrust/thrust/issues/692
//
//This specialization needs to be included before ANY thrust includes otherwise
//other device code inside thrust that calls it will not see it
namespace vtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{
//Forward declare of WrappedBinaryOperator
template <typename T, typename F>
class WrappedBinaryOperator;
}
}
}
} //namespace vtkm::exec::cuda::internal

namespace thrust
{
namespace system
{
namespace cuda
{
namespace detail
{
namespace bulk_
{
namespace detail
{
namespace accumulate_detail
{
template <typename ConcurrentGroup,
          typename RandomAccessIterator,
          typename Size,
          typename T,
          typename F>
__device__ T
destructive_accumulate_n(ConcurrentGroup& g,
                         RandomAccessIterator first,
                         Size n,
                         T init,
                         vtkm::exec::cuda::internal::WrappedBinaryOperator<T, F> binary_op)
{
  using size_type = typename ConcurrentGroup::size_type;

  size_type tid = g.this_exec.index();

  T x = init;
  if (tid < n)
  {
    x = first[tid];
  }

  g.wait();

  for (size_type offset = 1; offset < g.size(); offset += offset)
  {
    if (tid >= offset && tid - offset < n)
    {
      x = binary_op(first[tid - offset], x);
    }

    g.wait();

    if (tid < n)
    {
      first[tid] = x;
    }

    g.wait();
  }

  T result = binary_op(init, first[n - 1]);

  g.wait();

  return result;
}
}
}
} //namespace bulk_::detail::accumulate_detail
}
}
}
} //namespace thrust::system::cuda::detail

#endif

//So for thrust 1.9.0+ the aligned_reinterpret_cast has a bug
//where it is not marked as __host__device__. To fix this we add a new
//overload for void* with the correct markup (which is what everyone calls).
namespace thrust
{
namespace detail
{

//just in-case somebody has this fix also for primitive types
template <typename T, typename U>
T aligned_reinterpret_cast(U u);

#define ALIGN_RE_T(RT)                                                                             \
  template <>                                                                                      \
  inline __host__ __device__ RT* aligned_reinterpret_cast(void* u)                                 \
  {                                                                                                \
    return reinterpret_cast<RT*>(reinterpret_cast<void*>(u));                                      \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ RT* aligned_reinterpret_cast(vtkm::UInt8* u)                          \
  {                                                                                                \
    return reinterpret_cast<RT*>(reinterpret_cast<void*>(u));                                      \
  }                                                                                                \
  struct SwallowSemicolon

#define ALIGN_RE_VEC(RT, N)                                                                        \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, N>* aligned_reinterpret_cast(void* u)                   \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, N>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, N>* aligned_reinterpret_cast(vtkm::UInt8* u)            \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, N>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  struct SwallowSemicolon

#define ALIGN_RE_PAIR(T, U)                                                                        \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Pair<T, U>* aligned_reinterpret_cast(void* u)                   \
  {                                                                                                \
    return reinterpret_cast<vtkm::Pair<T, U>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Pair<T, U>* aligned_reinterpret_cast(vtkm::UInt8* u)            \
  {                                                                                                \
    return reinterpret_cast<vtkm::Pair<T, U>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  struct SwallowSemicolon

#ifndef VTKM_DONT_FIX_THRUST
ALIGN_RE_T(char);
ALIGN_RE_T(vtkm::Int8);
ALIGN_RE_T(vtkm::UInt8);
ALIGN_RE_T(vtkm::Int16);
ALIGN_RE_T(vtkm::UInt16);
ALIGN_RE_T(vtkm::Int32);
ALIGN_RE_T(vtkm::UInt32);
ALIGN_RE_T(vtkm::Int64);
ALIGN_RE_T(vtkm::UInt64);
ALIGN_RE_T(vtkm::Float32);
ALIGN_RE_T(vtkm::Float64);
#endif

ALIGN_RE_VEC(vtkm::UInt8, 3);
ALIGN_RE_VEC(vtkm::Int32, 3);
ALIGN_RE_VEC(vtkm::Int64, 3);
ALIGN_RE_VEC(vtkm::Float32, 3);
ALIGN_RE_VEC(vtkm::Float64, 3);

ALIGN_RE_VEC(vtkm::UInt8, 4);
ALIGN_RE_VEC(vtkm::Float32, 4);
ALIGN_RE_VEC(vtkm::Float64, 4);

ALIGN_RE_PAIR(vtkm::Int32, vtkm::Float32);
ALIGN_RE_PAIR(vtkm::Int32, vtkm::Float64);
ALIGN_RE_PAIR(vtkm::Int64, vtkm::Float32);
ALIGN_RE_PAIR(vtkm::Int64, vtkm::Float64);

#undef ALIGN_RE_T
#undef ALIGN_RE_VEC
#undef ALIGN_RE_PAIR
}
}

#endif //vtk_m_exec_cuda_internal_ThrustPatches_h
