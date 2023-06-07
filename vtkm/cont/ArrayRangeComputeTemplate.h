//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeComputeTemplate_h
#define vtk_m_cont_ArrayRangeComputeTemplate_h

#include <vtkm/cont/ArrayRangeCompute.h>

#include <vtkm/BinaryOperators.h>
#include <vtkm/Deprecated.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/Instantiations.h>

#include <vtkm/cont/Algorithm.h>

#include <limits>

namespace vtkm
{
namespace cont
{

namespace detail
{

struct ArrayRangeComputeFunctor
{
  template <typename Device, typename T, typename S>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, S>& handle,
                            const vtkm::Vec<T, 2>& initialValue,
                            vtkm::Vec<T, 2>& result) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;
    result = Algorithm::Reduce(handle, initialValue, vtkm::MinAndMax<T>());
    return true;
  }
};

} // namespace detail

namespace internal
{

template <typename T, typename S>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeGeneric(
  const vtkm::cont::ArrayHandle<T, S>& input,
  vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "ArrayRangeCompute");

  using VecTraits = vtkm::VecTraits<T>;
  using CT = typename VecTraits::ComponentType;
  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  vtkm::cont::ArrayHandle<vtkm::Range> range;
  range.Allocate(VecTraits::NUM_COMPONENTS);

  if (input.GetNumberOfValues() < 1)
  {
    auto portal = range.WritePortal();
    for (vtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
    {
      portal.Set(i, vtkm::Range());
    }
  }
  else
  {
    //We used the limits, so that we don't need to sync the array handle
    //
    vtkm::Vec<T, 2> result;
    vtkm::Vec<T, 2> initial;
    initial[0] = T(std::numeric_limits<CT>::max());
    initial[1] = T(std::numeric_limits<CT>::lowest());

    const bool rangeComputed = vtkm::cont::TryExecuteOnDevice(
      device, vtkm::cont::detail::ArrayRangeComputeFunctor{}, input, initial, result);
    if (!rangeComputed)
    {
      ThrowArrayRangeComputeFailed();
    }
    else
    {
      auto portal = range.WritePortal();
      for (vtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
      {
        portal.Set(i,
                   vtkm::Range(VecTraits::GetComponent(result[0], i),
                               VecTraits::GetComponent(result[1], i)));
      }
    }
  }
  return range;
}

template <typename S>
struct ArrayRangeComputeImpl
{
  template <typename T>
  vtkm::cont::ArrayHandle<vtkm::Range> operator()(const vtkm::cont::ArrayHandle<T, S>& input,
                                                  vtkm::cont::DeviceAdapterId device) const
  {
    return vtkm::cont::internal::ArrayRangeComputeGeneric(input, device);
  }
};

} // namespace internal


template <typename ArrayHandleType>
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeTemplate(
  const ArrayHandleType& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return internal::ArrayRangeComputeImpl<typename ArrayHandleType::StorageTag>{}(input, device);
}

template <typename ArrayHandleType>
VTKM_DEPRECATED(2.1, "Use precompiled ArrayRangeCompute or ArrayRangeComputeTemplate.")
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeTemplate(input, device);
}

}
} // namespace vtkm::cont

#define VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(modifiers, ...)                                     \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Int8, __VA_ARGS__>& input,                                \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::UInt8, __VA_ARGS__>& input,                               \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Int16, __VA_ARGS__>& input,                               \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::UInt16, __VA_ARGS__>& input,                              \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Int32, __VA_ARGS__>& input,                               \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::UInt32, __VA_ARGS__>& input,                              \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Int64, __VA_ARGS__>& input,                               \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::UInt64, __VA_ARGS__>& input,                              \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Float32, __VA_ARGS__>& input,                             \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Float64, __VA_ARGS__>& input,                             \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<bool, __VA_ARGS__>& input, vtkm::cont::DeviceAdapterId device); \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<char, __VA_ARGS__>& input, vtkm::cont::DeviceAdapterId device); \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<signed VTKM_UNUSED_INT_TYPE, __VA_ARGS__>& input,               \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<unsigned VTKM_UNUSED_INT_TYPE, __VA_ARGS__>& input,             \
    vtkm::cont::DeviceAdapterId device)

#define VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(modifiers, N, ...)                                     \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int8, N>, __VA_ARGS__>& input,                  \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<UInt8, N>, __VA_ARGS__>& input,                       \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<Int16, N>, __VA_ARGS__>& input,                       \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<UInt16, N>, __VA_ARGS__>& input,                      \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<Int32, N>, __VA_ARGS__>& input,                       \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<UInt32, N>, __VA_ARGS__>& input,                      \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<Int64, N>, __VA_ARGS__>& input,                       \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<UInt64, N>, __VA_ARGS__>& input,                      \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<Float32, N>, __VA_ARGS__>& input,                     \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<Float64, N>, __VA_ARGS__>& input,                     \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<bool, N>, __VA_ARGS__>& input,                        \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<char, N>, __VA_ARGS__>& input,                        \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<signed VTKM_UNUSED_INT_TYPE, N>, __VA_ARGS__>& input, \
    vtkm::cont::DeviceAdapterId device);                                                          \
  modifiers vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate(           \
    const vtkm::cont::ArrayHandle<vtkm::Vec<unsigned VTKM_UNUSED_INT_TYPE, N>, __VA_ARGS__>&      \
      input,                                                                                      \
    vtkm::cont::DeviceAdapterId device)

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                      vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagSOA);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagSOA);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagSOA);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(
  extern template VTKM_CONT_TEMPLATE_EXPORT,
  3,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                      vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END

#endif //vtk_m_cont_ArrayRangeComputeTemplate_h
