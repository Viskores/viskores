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

//Forward declare of WrappedBinaryOperator
namespace vtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

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
//So for thrust 1.8.0 - 1.8.2 the inclusive_scan has a bug when accumulating
//values when the binary operators states it is not commutative.
//For more complex value types, we patch thrust/bulk with fix that is found
//in issue: https://github.com/thrust/thrust/issues/692
//
//This specialization needs to be included before ANY thrust includes otherwise
//other device code inside thrust that calls it will not see it
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

#endif //vtk_m_exec_cuda_internal_ThrustPatches_h
