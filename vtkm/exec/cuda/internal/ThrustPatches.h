//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_ThrustPatches_h
#define vtk_m_exec_cuda_internal_ThrustPatches_h

#include <vtkm/Types.h>

#ifdef VTKM_ENABLE_CUDA

// Needed so we can conditionally include components
#include <thrust/version.h>

#if THRUST_VERSION >= 100900 && THRUST_VERSION < 100906
//So for thrust 1.9.0+ ( CUDA 9.X+ ) the aligned_reinterpret_cast has a bug
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

#define ALIGN_RE_VEC(RT)                                                                           \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, 2>* aligned_reinterpret_cast(void* u)                   \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, 2>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, 3>* aligned_reinterpret_cast(void* u)                   \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, 3>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, 4>* aligned_reinterpret_cast(void* u)                   \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, 4>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<vtkm::Vec<RT, 3>, 2>* aligned_reinterpret_cast(void* u)     \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<vtkm::Vec<RT, 3>, 2>*>(reinterpret_cast<void*>(u));          \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<vtkm::Vec<RT, 9>, 2>* aligned_reinterpret_cast(void* u)     \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<vtkm::Vec<RT, 9>, 2>*>(reinterpret_cast<void*>(u));          \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, 2>* aligned_reinterpret_cast(vtkm::UInt8* u)            \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, 2>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, 3>* aligned_reinterpret_cast(vtkm::UInt8* u)            \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, 3>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<RT, 4>* aligned_reinterpret_cast(vtkm::UInt8* u)            \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<RT, 4>*>(reinterpret_cast<void*>(u));                        \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<vtkm::Vec<RT, 2>, 2>* aligned_reinterpret_cast(             \
    vtkm::UInt8* u)                                                                                \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<vtkm::Vec<RT, 2>, 2>*>(reinterpret_cast<void*>(u));          \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<vtkm::Vec<RT, 3>, 2>* aligned_reinterpret_cast(             \
    vtkm::UInt8* u)                                                                                \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<vtkm::Vec<RT, 3>, 2>*>(reinterpret_cast<void*>(u));          \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<vtkm::Vec<RT, 4>, 2>* aligned_reinterpret_cast(             \
    vtkm::UInt8* u)                                                                                \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<vtkm::Vec<RT, 4>, 2>*>(reinterpret_cast<void*>(u));          \
  }                                                                                                \
  template <>                                                                                      \
  inline __host__ __device__ vtkm::Vec<vtkm::Vec<RT, 9>, 2>* aligned_reinterpret_cast(             \
    vtkm::UInt8* u)                                                                                \
  {                                                                                                \
    return reinterpret_cast<vtkm::Vec<vtkm::Vec<RT, 9>, 2>*>(reinterpret_cast<void*>(u));          \
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
ALIGN_RE_T(bool);
ALIGN_RE_T(char);
ALIGN_RE_T(vtkm::Int8);
ALIGN_RE_T(vtkm::UInt8);
ALIGN_RE_T(vtkm::Int16);
ALIGN_RE_T(vtkm::UInt16);
ALIGN_RE_T(vtkm::Int32);
ALIGN_RE_T(vtkm::UInt32);
// Need these for vtk. don't need long long, since those are used for [U]Int64.
ALIGN_RE_T(long);
ALIGN_RE_T(unsigned long);
ALIGN_RE_T(vtkm::Int64);
ALIGN_RE_T(vtkm::UInt64);
ALIGN_RE_T(vtkm::Float32);
ALIGN_RE_T(vtkm::Float64);
#endif

ALIGN_RE_VEC(char);
ALIGN_RE_VEC(vtkm::Int8);
ALIGN_RE_VEC(vtkm::UInt8);
ALIGN_RE_VEC(vtkm::Int16);
ALIGN_RE_VEC(vtkm::UInt16);
ALIGN_RE_VEC(vtkm::Int32);
ALIGN_RE_VEC(vtkm::UInt32);
// Need these for vtk. don't need long long, since those are used for [U]Int64.
ALIGN_RE_VEC(long);
ALIGN_RE_VEC(unsigned long);
ALIGN_RE_VEC(vtkm::Int64);
ALIGN_RE_VEC(vtkm::UInt64);
ALIGN_RE_VEC(vtkm::Float32);
ALIGN_RE_VEC(vtkm::Float64);

ALIGN_RE_PAIR(vtkm::Int32, vtkm::Int32);
ALIGN_RE_PAIR(vtkm::Int32, vtkm::Int64);
ALIGN_RE_PAIR(vtkm::Int32, vtkm::Float32);
ALIGN_RE_PAIR(vtkm::Int32, vtkm::Float64);

ALIGN_RE_PAIR(vtkm::Int64, vtkm::Int32);
ALIGN_RE_PAIR(vtkm::Int64, vtkm::Int64);
ALIGN_RE_PAIR(vtkm::Int64, vtkm::Float32);
ALIGN_RE_PAIR(vtkm::Int64, vtkm::Float64);

#undef ALIGN_RE_T
#undef ALIGN_RE_VEC
#undef ALIGN_RE_PAIR
}
}
#endif //THRUST_VERSION >= 100900

#if THRUST_VERSION >= 100904
//So for thrust 1.9.4+ (CUDA 10.1+) the stateless_resource_allocator has a bug
//where it is not marked as __host__ __device__ && __thrust_exec_check_disable__.
//To fix this we add a new partial specialization on cuda::memory_resource
//which the correct markup (which is what everyone calls).
//See: https://github.com/thrust/thrust/issues/972
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/mr/allocator.h>
#include <thrust/system/cuda/memory_resource.h>
VTKM_THIRDPARTY_POST_INCLUDE
namespace thrust
{
namespace mr
{

template <typename T>
class stateless_resource_allocator<T, ::thrust::system::cuda::memory_resource>
  : public thrust::mr::allocator<T, ::thrust::system::cuda::memory_resource>
{
  typedef ::thrust::system::cuda::memory_resource Upstream;
  typedef thrust::mr::allocator<T, Upstream> base;

public:
  /*! The \p rebind metafunction provides the type of an \p stateless_resource_allocator instantiated with another type.
     *
     *  \tparam U the other type to use for instantiation.
     */
  template <typename U>
  struct rebind
  {
    /*! The typedef \p other gives the type of the rebound \p stateless_resource_allocator.
         */
    typedef stateless_resource_allocator<U, Upstream> other;
  };

  /*! Default constructor. Uses \p get_global_resource to get the global instance of \p Upstream and initializes the
     *      \p allocator base subobject with that resource.
     */
  __thrust_exec_check_disable__ //modification, required to suppress warnings
    __host__ __device__         //modification, required to suppress warnings
    stateless_resource_allocator()
    : base(get_global_resource<Upstream>())
  {
  }

  /*! Copy constructor. Copies the memory resource pointer. */
  __host__ __device__ stateless_resource_allocator(const stateless_resource_allocator& other)
    : base(other)
  {
  }

  /*! Conversion constructor from an allocator of a different type. Copies the memory resource pointer. */
  template <typename U>
  __host__ __device__
  stateless_resource_allocator(const stateless_resource_allocator<U, Upstream>& other)
    : base(other)
  {
  }

  /*! Destructor. */
  __host__ __device__ ~stateless_resource_allocator() {}
};
}
}
#endif //THRUST_VERSION >= 100903


#if THRUST_VERSION < 100900
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
#endif //THRUST_VERSION < 100900

#endif //CUDA enabled

#endif //vtk_m_exec_cuda_internal_ThrustPatches_h
