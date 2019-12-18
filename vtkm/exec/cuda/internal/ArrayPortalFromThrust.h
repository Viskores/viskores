//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h
#define vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>

#include <iterator>
#include <type_traits>

#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system/cuda/memory.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

// The clang-format rules want to put the curly braces on separate lines. Since
// these declarations are a type-level truth table, minimize the amount of
// space it takes up.
// clang-format off
template <typename T> struct UseScalarTextureLoad : public std::false_type {};
template <typename T> struct UseVecTextureLoads : public std::false_type {};
template <typename T> struct UseMultipleScalarTextureLoads : public std::false_type {};

//currently CUDA doesn't support texture loading of signed char's so that is why
//you don't see vtkm::Int8 in any of the lists.
template <> struct UseScalarTextureLoad<const vtkm::UInt8> : std::true_type {};
template <> struct UseScalarTextureLoad<const vtkm::Int16> : std::true_type {};
template <> struct UseScalarTextureLoad<const vtkm::UInt16> : std::true_type {};
template <> struct UseScalarTextureLoad<const vtkm::Int32> : std::true_type {};
template <> struct UseScalarTextureLoad<const vtkm::UInt32> : std::true_type {};
template <> struct UseScalarTextureLoad<const vtkm::Float32> : std::true_type {};
template <> struct UseScalarTextureLoad<const vtkm::Float64> : std::true_type {};

//CUDA needs vec types converted to CUDA types ( float2, uint2), so we have a special
//case for these vec texture loads.
template <> struct UseVecTextureLoads<const vtkm::Vec2i_32> : std::true_type {};
template <> struct UseVecTextureLoads<const vtkm::Vec2ui_32> : std::true_type {};
template <> struct UseVecTextureLoads<const vtkm::Vec2f_32> : std::true_type {};
template <> struct UseVecTextureLoads<const vtkm::Vec2f_64> : std::true_type {};

template <> struct UseVecTextureLoads<const vtkm::Vec4i_32> : std::true_type {};
template <> struct UseVecTextureLoads<const vtkm::Vec4ui_32> : std::true_type {};
template <> struct UseVecTextureLoads<const vtkm::Vec4f_32> : std::true_type {};

//CUDA doesn't support loading 3 wide values through a texture unit by default,
//so instead we fetch through texture three times and store the result
//currently CUDA doesn't support texture loading of signed char's so that is why
//you don't see vtkm::Int8 in any of the lists.

template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec2ui_8> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec2i_16> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec2ui_16> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec2i_64> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec2ui_64> : std::true_type {};

template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3ui_8> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3i_16> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3ui_16> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3i_32> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3ui_32> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3f_32> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec3f_64> : std::true_type {};

template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec4ui_8> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec4i_16> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec4ui_16> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec4i_64> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec4ui_64> : std::true_type {};
template <> struct UseMultipleScalarTextureLoads<const vtkm::Vec4f_64> : std::true_type {};
// clang-format on

//this T type is not one that is valid to be loaded through texture memory
template <typename T, typename Enable = void>
struct load_through_texture
{
  static constexpr vtkm::IdComponent WillUseTexture = 0;

  __device__ static T get(const T* const data) { return *data; }
};

//only load through a texture if we have sm 35 support

// this T type is valid to be loaded through a single texture memory fetch
template <typename T>
struct load_through_texture<T, typename std::enable_if<UseScalarTextureLoad<const T>::value>::type>
{

  static constexpr vtkm::IdComponent WillUseTexture = 1;

  __device__ static T get(const T* const data)
  {
#if __CUDA_ARCH__ >= 350
    // printf("__CUDA_ARCH__ UseScalarTextureLoad");
    return __ldg(data);
#else
    return *data;
#endif
  }
};

// this T type is valid to be loaded through a single vec texture memory fetch
template <typename T>
struct load_through_texture<T, typename std::enable_if<UseVecTextureLoads<const T>::value>::type>
{
  static constexpr vtkm::IdComponent WillUseTexture = 1;

  __device__ static T get(const T* const data)
  {
#if __CUDA_ARCH__ >= 350
    // printf("__CUDA_ARCH__ UseVecTextureLoads");
    return getAs(data);
#else
    return *data;
#endif
  }

  __device__ static vtkm::Vec2i_32 getAs(const vtkm::Vec2i_32* const data)
  {
    const int2 temp = __ldg((const int2*)data);
    return vtkm::Vec2i_32(temp.x, temp.y);
  }

  __device__ static vtkm::Vec2ui_32 getAs(const vtkm::Vec2ui_32* const data)
  {
    const uint2 temp = __ldg((const uint2*)data);
    return vtkm::Vec2ui_32(temp.x, temp.y);
  }

  __device__ static vtkm::Vec4i_32 getAs(const vtkm::Vec4i_32* const data)
  {
    const int4 temp = __ldg((const int4*)data);
    return vtkm::Vec4i_32(temp.x, temp.y, temp.z, temp.w);
  }

  __device__ static vtkm::Vec4ui_32 getAs(const vtkm::Vec4ui_32* const data)
  {
    const uint4 temp = __ldg((const uint4*)data);
    return vtkm::Vec4ui_32(temp.x, temp.y, temp.z, temp.w);
  }

  __device__ static vtkm::Vec2f_32 getAs(const vtkm::Vec2f_32* const data)
  {
    const float2 temp = __ldg((const float2*)data);
    return vtkm::Vec2f_32(temp.x, temp.y);
  }

  __device__ static vtkm::Vec4f_32 getAs(const vtkm::Vec4f_32* const data)
  {
    const float4 temp = __ldg((const float4*)data);
    return vtkm::Vec4f_32(temp.x, temp.y, temp.z, temp.w);
  }

  __device__ static vtkm::Vec2f_64 getAs(const vtkm::Vec2f_64* const data)
  {
    const double2 temp = __ldg((const double2*)data);
    return vtkm::Vec2f_64(temp.x, temp.y);
  }
};

//this T type is valid to be loaded through multiple texture memory fetches
template <typename T>
struct load_through_texture<
  T,
  typename std::enable_if<UseMultipleScalarTextureLoads<const T>::value>::type>
{
  static constexpr vtkm::IdComponent WillUseTexture = 1;

  using NonConstT = typename std::remove_const<T>::type;

  __device__ static T get(const T* const data)
  {
#if __CUDA_ARCH__ >= 350
    // printf("__CUDA_ARCH__ UseMultipleScalarTextureLoads");
    return getAs(data);
#else
    return *data;
#endif
  }

  __device__ static T getAs(const T* const data)
  {
    //we need to fetch each component individually
    const vtkm::IdComponent NUM_COMPONENTS = T::NUM_COMPONENTS;
    using ComponentType = typename T::ComponentType;
    const ComponentType* recasted_data = (const ComponentType*)(data);
    NonConstT result;
#pragma unroll
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = __ldg(recasted_data + i);
    }
    return result;
  }
};

class ArrayPortalFromThrustBase
{
};

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template <typename T>
class ArrayPortalFromThrust : public ArrayPortalFromThrustBase
{
public:
  using ValueType = T;
  using IteratorType = T*;
  using difference_type = std::ptrdiff_t;

  VTKM_EXEC_CONT ArrayPortalFromThrust() {}

  VTKM_CONT
  ArrayPortalFromThrust(IteratorType begin, IteratorType end)
    : BeginIterator(begin)
    , EndIterator(end)
  {
  }

  /// Copy constructor for any other ArrayPortalFromThrust with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <typename OtherT>
  VTKM_EXEC_CONT ArrayPortalFromThrust(const ArrayPortalFromThrust<OtherT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , EndIterator(src.GetIteratorEnd())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return static_cast<vtkm::Id>((this->EndIterator - this->BeginIterator));
  }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return *(this->BeginIterator + static_cast<difference_type>(index));
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, ValueType value) const
  {
    *(this->BeginIterator + static_cast<difference_type>(index)) = value;
  }

  VTKM_EXEC_CONT
  IteratorType GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_EXEC_CONT
  IteratorType GetIteratorEnd() const { return this->EndIterator; }

private:
  IteratorType BeginIterator;
  IteratorType EndIterator;
};

template <typename T>
class ConstArrayPortalFromThrust : public ArrayPortalFromThrustBase
{
public:
  using ValueType = T;
  using IteratorType = const T*;
  using difference_type = std::ptrdiff_t;

  VTKM_EXEC_CONT ConstArrayPortalFromThrust()
    : BeginIterator(nullptr)
    , EndIterator(nullptr)
  {
  }

  VTKM_CONT
  ConstArrayPortalFromThrust(IteratorType begin, IteratorType end)
    : BeginIterator(begin)
    , EndIterator(end)
  {
    // printf("ConstArrayPortalFromThrust() %s \n", __PRETTY_FUNCTION__ );
  }

  /// Copy constructor for any other ConstArrayPortalFromThrust with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  // template<typename OtherT>
  VTKM_EXEC_CONT
  ConstArrayPortalFromThrust(const ArrayPortalFromThrust<T>& src)
    : BeginIterator(src.GetIteratorBegin())
    , EndIterator(src.GetIteratorEnd())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return static_cast<vtkm::Id>((this->EndIterator - this->BeginIterator));
  }

//The VTKM_CUDA_DEVICE_PASS define makes sure that the device only signature
//only shows up for the device compilation. This allows the nvcc compiler
//to have separate host and device code paths for the same method. This
//solves the problem of trying to call a device only method from a
//device/host method
#ifdef VTKM_CUDA_DEVICE_PASS
  __device__ ValueType Get(vtkm::Id index) const
  {
    return vtkm::exec::cuda::internal::load_through_texture<ValueType>::get(this->BeginIterator +
                                                                            index);
  }

  __device__ void Set(vtkm::Id vtkmNotUsed(index), ValueType vtkmNotUsed(value)) const {}

#else
  ValueType Get(vtkm::Id vtkmNotUsed(index)) const { return ValueType(); }

  void Set(vtkm::Id vtkmNotUsed(index), ValueType vtkmNotUsed(value)) const
  {
#if !(defined(VTKM_MSVC) && defined(VTKM_CUDA))
    VTKM_ASSERT(true && "Cannot set to const array.");
#endif
  }
#endif

  VTKM_EXEC_CONT
  IteratorType GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_EXEC_CONT
  IteratorType GetIteratorEnd() const { return this->EndIterator; }

private:
  IteratorType BeginIterator;
  IteratorType EndIterator;
};
}
}
}
} // namespace vtkm::exec::cuda::internal

#endif //vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h
