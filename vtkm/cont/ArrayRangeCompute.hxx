//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeCompute_hxx
#define vtk_m_cont_ArrayRangeCompute_hxx

#include <vtkm/cont/ArrayRangeCompute.h>

#include <vtkm/BinaryOperators.h>
#include <vtkm/VecTraits.h>

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

template <typename T, typename S>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeImpl(
  const vtkm::cont::ArrayHandle<T, S>& input,
  vtkm::cont::DeviceAdapterId device)
{
  using VecTraits = vtkm::VecTraits<T>;
  using CT = typename VecTraits::ComponentType;
  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  vtkm::cont::ArrayHandle<vtkm::Range> range;
  range.Allocate(VecTraits::NUM_COMPONENTS);

  if (input.GetNumberOfValues() < 1)
  {
    auto portal = range.GetPortalControl();
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
      device, detail::ArrayRangeComputeFunctor{}, input, initial, result);
    if (!rangeComputed)
    {
      ThrowArrayRangeComputeFailed();
    }
    else
    {
      auto portal = range.GetPortalControl();
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

} // namespace detail


VTKM_CONT
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>& input,
  vtkm::cont::DeviceAdapterId device)
{
  using UniformHandleType = ArrayHandleUniformPointCoordinates;
  using RectilinearHandleType =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

  if (input.IsType<UniformHandleType>())
  {
    using T = typename UniformHandleType::ValueType;
    using S = typename UniformHandleType::StorageTag;
    const vtkm::cont::internal::detail::StorageVirtual* storage =
      input.GetStorage().GetStorageVirtual();
    const auto* castStorage =
      storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();

    return ArrayRangeCompute(castStorage->GetHandle(), device);
  }
  else if (input.IsType<RectilinearHandleType>())
  {
    using T = typename RectilinearHandleType::ValueType;
    using S = typename RectilinearHandleType::StorageTag;
    const vtkm::cont::internal::detail::StorageVirtual* storage =
      input.GetStorage().GetStorageVirtual();
    const auto* castStorage =
      storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();

    return ArrayRangeCompute(castStorage->GetHandle(), device);
  }
  else
  {
    return detail::ArrayRangeComputeImpl(input, device);
  }
}

template <typename ArrayHandleType>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(const ArrayHandleType& input,
                                                              vtkm::cont::DeviceAdapterId device)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return detail::ArrayRangeComputeImpl(input, device);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayRangeCompute_hxx
