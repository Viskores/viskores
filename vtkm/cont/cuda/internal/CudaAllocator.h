//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_cont_cuda_internal_CudaAllocator_h
#define vtk_m_cont_cuda_internal_CudaAllocator_h

#include <vtkm/cont/vtkm_cont_export.h>
#include <vtkm/internal/ExportMacros.h>

#include <cstddef>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

/// Collection of cuda-specific memory management operations.
struct VTKM_CONT_EXPORT CudaAllocator
{
  /// Returns true if all detected CUDA devices support pageable managed memory
  /// that can be accessed concurrently by the CPU and GPUs.
  static VTKM_CONT bool UsingManagedMemory();

  /// Returns true if the pointer is accessible from a CUDA device.
  static VTKM_CONT bool IsDevicePointer(const void* ptr);

  /// Returns true if the pointer is a CUDA pointer allocated with
  /// cudaMallocManaged.
  static VTKM_CONT bool IsManagedPointer(const void* ptr);

  static VTKM_CONT void* Allocate(std::size_t numBytes);
  static VTKM_CONT void Free(void* ptr);

  static VTKM_CONT void PrepareForControl(const void* ptr, std::size_t numBytes);

  static VTKM_CONT void PrepareForInput(const void* ptr, std::size_t numBytes);
  static VTKM_CONT void PrepareForOutput(const void* ptr, std::size_t numBytes);
  static VTKM_CONT void PrepareForInPlace(const void* ptr, std::size_t numBytes);

private:
  static VTKM_CONT void Initialize();
};
}
}
}
} // end namespace vtkm::cont::cuda::internal

#endif // vtk_m_cont_cuda_internal_CudaAllocator_h
