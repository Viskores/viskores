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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

/******************************************************************************
 * Copyright (c) 2011, Duane Merrill.  All rights reserved.
 * Copyright (c) 2011-2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef vtk_m_exec_cuda_internal_ArrayPortalFromTexture_h
#define vtk_m_exec_cuda_internal_ArrayPortalFromTexture_h

#include <vtkm/Types.h>

#include <iterator>

// Disable GCC warnings we check vtkmfor but Thrust does not.
#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif // gcc version >= 4.6
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 2)
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // gcc version >= 4.2
#endif // gcc && !CUDA

#include <thrust/system/cuda/memory.h>
#include <thrust/iterator/iterator_facade.h>

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA


namespace vtkm {
namespace exec {
namespace cuda {
namespace internal {

template<typename T>
class ConstArrayPortalFromTexture : public ArrayPortalFromThrustBase
{
public:

  typedef T ValueType;
  typedef typename thrust::system::cuda::pointer< T > PointerType;
  typedef const T* IteratorType;

  VTKM_EXEC_CONT_EXPORT ConstArrayPortalFromTexture() {  }

  VTKM_CONT_EXPORT
  ConstArrayPortalFromTexture(const PointerType begin, const PointerType end)
    : BeginIterator( begin ),
      EndIterator( end )
      {  }

  /// Copy constructor for any other ConstArrayPortalFromTexture with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromTexture(const ConstArrayPortalFromTexture<OtherT> &src)
    : BeginIterator(src.BeginIterator),
      EndIterator(src.EndIterator)
  {  }

  template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromTexture<T> &operator=(
      const ConstArrayPortalFromTexture<OtherT> &src)
  {
    this->BeginIterator = src.BeginIterator;
    this->EndIterator = src.EndIterator;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return (this->EndIterator - this->BeginIterator);
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
#if __CUDA_ARCH__ >= 350
    return __ldg(this->IteratorAt(index).get());
#else
    return *this->IteratorAt(index);
#endif
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, ValueType value) const {
    *this->IteratorAt(index) = value;
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorBegin() const { return this->BeginIterator.get(); }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorEnd() const { return this->EndIterator.get(); }

private:
  PointerType BeginIterator;
  PointerType EndIterator;

  VTKM_EXEC_EXPORT
  PointerType IteratorAt(vtkm::Id index) const {
    // Not using std::advance because on CUDA it cannot be used on a device.
    return (this->BeginIterator + index);
  }
};

}
}
}
} // namespace vtkm::exec::cuda::internal


#endif //vtk_m_exec_cuda_internal_ArrayPortalFromTexture_h
