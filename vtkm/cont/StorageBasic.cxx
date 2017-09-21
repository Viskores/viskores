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

#define vtkm_cont_StorageBasic_cxx
#include <vtkm/cont/StorageBasic.h>

#if defined(VTKM_POSIX)
#define VTKM_MEMALIGN_POSIX
#elif defined(_WIN32)
#define VTKM_MEMALIGN_WIN
#elif defined(__SSE__)
#define VTKM_MEMALIGN_SSE
#else
#define VTKM_MEMALIGN_NONE
#endif

#if defined(VTKM_MEMALIGN_POSIX)
#include <stdlib.h>
#elif defined(VTKM_MEMALIGN_WIN)
#include <malloc.h>
#elif defined(VTKM_MEMALIGN_SSE)
#include <xmmintrin.h>
#else
#include <malloc.h>
#endif

#include <cstddef>
#include <cstdlib>

namespace vtkm
{
namespace cont
{
namespace internal
{

StorageBasicBase::~StorageBasicBase()
{
}

void* alloc_aligned(size_t size, size_t align)
{
#if defined(VTKM_MEMALIGN_POSIX)
  void* mem = nullptr;
  if (posix_memalign(&mem, align, size) != 0)
  {
    mem = nullptr;
  }
#elif defined(VTKM_MEMALIGN_WIN)
  void* mem = _aligned_malloc(size, align);
#elif defined(VTKM_MEMALIGN_SSE)
  void* mem = _mm_malloc(size, align);
#else
  void* mem = malloc(size);
#endif
  if (mem == nullptr)
  {
    throw std::bad_alloc();
  }
  return mem;
}

void free_aligned(void* mem)
{
#if defined(VTKM_MEMALIGN_POSIX)
  free(mem);
#elif defined(VTKM_MEMALIGN_WIN)
  _aligned_free(mem);
#elif defined(VTKM_MEMALIGN_SSE)
  _mm_free(mem);
#else
  free(mem);
#endif
}

template class VTKM_CONT_EXPORT Storage<char, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int8, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt8, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int16, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt16, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int32, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt32, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int64, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt64, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Float32, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Float64, StorageTagBasic>;

template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Int64, 2>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Int32, 2>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Float32, 2>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Float64, 2>, StorageTagBasic>;

template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Int64, 3>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Int32, 3>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Float32, 3>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Float64, 3>, StorageTagBasic>;

template class VTKM_CONT_EXPORT Storage<vtkm::Vec<char, 4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<Int8, 4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<UInt8, 4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Float32, 4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT Storage<vtkm::Vec<vtkm::Float64, 4>, StorageTagBasic>;
}
}
} // namespace vtkm::cont::internal
