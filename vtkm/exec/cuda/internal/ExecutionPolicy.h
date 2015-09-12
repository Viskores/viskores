
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
#ifndef vtk_m_exec_cuda_internal_ExecutionPolicy_h
#define vtk_m_exec_cuda_internal_ExecutionPolicy_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/exec/cuda/internal/WrappedOperators.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/execution_policy.h>
#include <thrust/system/cuda/execution_policy.h>
#include <thrust/system/cuda/memory.h>
VTKM_THIRDPARTY_POST_INCLUDE


struct vtkm_cuda_policy : thrust::device_execution_policy<vtkm_cuda_policy> {};


//Specialize the sort call for cuda pointers using less/greater operators.
//The purpose of this is that for 32bit types (UInt32,Int32,Float32) thrust
//will call a super fast radix sort only if the operator is thrust::less
//or thrust::greater.
__thrust_hd_warning_disable__
template<typename T>
__host__ __device__
  void sort(const vtkm_cuda_policy &exec,
            thrust::system::cuda::pointer<T> first,
            thrust::system::cuda::pointer<T> last,
            vtkm::exec::cuda::internal::WrappedBinaryPredicate<T, vtkm::SortLess > comp)
{  //sort for concrete pointers and less than op
   //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(thrust::cuda::par, first, last, thrust::less<T>());
}

__thrust_hd_warning_disable__
template<typename T>
__host__ __device__
  void sort(const vtkm_cuda_policy &exec,
            thrust::system::cuda::pointer<T> first,
            thrust::system::cuda::pointer<T> last,
            vtkm::exec::cuda::internal::WrappedBinaryPredicate<T, ::thrust::less<T> > comp)
{ //sort for concrete pointers and less than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(thrust::cuda::par, first, last, thrust::less<T>());
}

__thrust_hd_warning_disable__
template<typename T>
__host__ __device__
  void sort(const vtkm_cuda_policy &exec,
            thrust::system::cuda::pointer<T> first,
            thrust::system::cuda::pointer<T> last,
            vtkm::exec::cuda::internal::WrappedBinaryPredicate<T, vtkm::SortGreater> comp)
{ //sort for concrete pointers and greater than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(thrust::cuda::par, first, last, thrust::greater<T>());
}

__thrust_hd_warning_disable__
template<typename T>
__host__ __device__
  void sort(const vtkm_cuda_policy &exec,
            thrust::system::cuda::pointer<T> first,
            thrust::system::cuda::pointer<T> last,
            vtkm::exec::cuda::internal::WrappedBinaryPredicate<T, ::thrust::greater<T> > comp)
{ //sort for concrete pointers and greater than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(thrust::cuda::par, first, last, thrust::greater<T>());
}

__thrust_hd_warning_disable__
template<typename RandomAccessIterator,
         typename StrictWeakOrdering>
__host__ __device__
  void sort(const vtkm_cuda_policy &exec,
            RandomAccessIterator first,
            RandomAccessIterator last,
            StrictWeakOrdering comp)
{
  //At this point the pointer type is not a cuda pointers and/or
  //the operator is not an approved less/greater operator.
  //This most likely will cause thrust to internally determine that
  //the best sort implementation is merge sort.
  return thrust::sort(thrust::cuda::par, first, last, comp);
}


#endif
