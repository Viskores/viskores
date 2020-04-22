//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_ArrayPortalBasicCuda_h
#define vtk_m_exec_cuda_internal_ArrayPortalBasicCuda_h

// This file provides specializations of ArrayPortalBasic that use texture loading
// intrinsics to load data from arrays faster in read-only arrays. These intrinsics
// are only available with compute capabilities >= 3.5, so only compile this code if
// we are compiling for that.
#if __CUDA_ARCH__ >= 350

#include <vtkm/Types.h>

namespace vtkm
{
namespace internal
{
namespace detail
{

// Forward declaration (declared in vtkm/internal/ArrayPortalBasic.h)
template <typename T>
VTKM_EXEC_CONT static inline T ArrayPortalBasicReadGet(const T* const data);

// Use the __ldg intrinsic to load read-only arrays through texture reads.
// Currently CUDA doesn't support texture loading of signed char's so that is why
// You don't see vtkm::Int8 in any of the lists.

VTKM_EXEC_CONT static inline vtkm::UInt8 ArrayPortalBasicReadGet(const vtkm::UInt8* const data)
{
  return __ldg(data);
}
VTKM_EXEC_CONT static inline vtkm::Int16 ArrayPortalBasicReadGet(const vtkm::Int16* const data)
{
  return __ldg(data);
}
VTKM_EXEC_CONT static inline vtkm::UInt16 ArrayPortalBasicReadGet(const vtkm::UInt16* const data)
{
  return __ldg(data);
}
VTKM_EXEC_CONT static inline vtkm::Int32 ArrayPortalBasicReadGet(const vtkm::Int32* const data)
{
  return __ldg(data);
}
VTKM_EXEC_CONT static inline vtkm::UInt32 ArrayPortalBasicReadGet(const vtkm::UInt32* const data)
{
  return __ldg(data);
}
VTKM_EXEC_CONT static inline vtkm::Float32 ArrayPortalBasicReadGet(const vtkm::Float32* const data)
{
  return __ldg(data);
}
VTKM_EXEC_CONT static inline vtkm::Float64 ArrayPortalBasicReadGet(const vtkm::Float64* const data)
{
  return __ldg(data);
}

// CUDA can do some vector texture loads, but only for its own types, so we have to convert
// to the CUDA type first.

VTKM_EXEC_CONT static inline vtkm::Vec2i_32 ArrayPortalBasicReadGet(
  const vtkm::Vec2i_32* const data)
{
  const int2 temp = __ldg(reinterpret_cast<const int2*>(data));
  return vtkm::Vec2i_32(temp.x, temp.y);
}
VTKM_EXEC_CONT static inline vtkm::Vec2ui_32 ArrayPortalBasicReadGet(
  const vtkm::Vec2ui_32* const data)
{
  const uint2 temp = __ldg(reinterpret_cast<const uint2*>(data));
  return vtkm::Vec2ui_32(temp.x, temp.y);
}
VTKM_EXEC_CONT static inline vtkm::Vec2f_32 ArrayPortalBasicReadGet(
  const vtkm::Vec2f_32* const data)
{
  const float2 temp = __ldg(reinterpret_cast<const float2*>(data));
  return vtkm::Vec2f_32(temp.x, temp.y);
}
VTKM_EXEC_CONT static inline vtkm::Vec2f_64 ArrayPortalBasicReadGet(
  const vtkm::Vec2f_64* const data)
{
  const double2 temp = __ldg(reinterpret_cast<const double2*>(data));
  return vtkm::Vec2f_64(temp.x, temp.y);
}

VTKM_EXEC_CONT static inline vtkm::Vec4i_32 ArrayPortalBasicReadGet(
  const vtkm::Vec4i_32* const data)
{
  const int4 temp = __ldg(reinterpret_cast<const int4*>(data));
  return vtkm::Vec4i_32(temp.x, temp.y, temp.z, temp.w);
}
VTKM_EXEC_CONT static inline vtkm::Vec4ui_32 ArrayPortalBasicReadGet(
  const vtkm::Vec4ui_32* const data)
{
  const uint4 temp = __ldg(reinterpret_cast<const uint4*>(data));
  return vtkm::Vec4ui_32(temp.x, temp.y, temp.z, temp.w);
}
VTKM_EXEC_CONT static inline vtkm::Vec4f_32 ArrayPortalBasicReadGet(
  const vtkm::Vec4f_32* const data)
{
  const float4 temp = __ldg(reinterpret_cast<const float4*>(data));
  return vtkm::Vec4f_32(temp.x, temp.y, temp.z, temp.w);
}

// CUDA does not support loading many of the vector types we use including 3-wide vectors.
// Support these using multiple scalar loads.

template <typename T, vtkm::IdComponent N>
VTKM_EXEC_CONT static inline vtkm::Vec<T, N> ArrayPortalBasicReadGet(
  const vtkm::Vec<T, N>* const data)
{
  const T* recastedData = reinterpret_cast<const T*>(data);
  vtkm::Vec<T, N> result;
#pragma unroll
  for (vtkm::IdComponent i = 0; i < N; ++i)
  {
    result[i] = ArrayPortalBasicReadGet(recastedData + i);
  }
  return result;
}
}
}
} // namespace vtkm::internal::detail

#endif // __CUDA_ARCH__ >= 350

#endif //vtk_m_exec_cuda_internal_ArrayPortalBasicCuda_h
