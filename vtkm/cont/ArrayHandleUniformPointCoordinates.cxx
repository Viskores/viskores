//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/ArrayRangeComputeTemplate.h>

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace cont
{

ArrayHandleUniformPointCoordinates::ArrayHandleUniformPointCoordinates(vtkm::Id3 dimensions,
                                                                       ValueType origin,
                                                                       ValueType spacing)
  : Superclass(internal::PortalToArrayHandleImplicitBuffers(
      vtkm::internal::ArrayPortalUniformPointCoordinates(dimensions, origin, spacing)))
{
}

ArrayHandleUniformPointCoordinates::~ArrayHandleUniformPointCoordinates() = default;

vtkm::Id3 ArrayHandleUniformPointCoordinates::GetDimensions() const
{
  return this->ReadPortal().GetDimensions();
}

vtkm::Vec3f ArrayHandleUniformPointCoordinates::GetOrigin() const
{
  return this->ReadPortal().GetOrigin();
}

vtkm::Vec3f ArrayHandleUniformPointCoordinates::GetSpacing() const
{
  return this->ReadPortal().GetSpacing();
}

namespace internal
{

vtkm::cont::ArrayHandleStride<vtkm::FloatDefault>
ArrayExtractComponentImpl<vtkm::cont::StorageTagUniformPoints>::operator()(
  const vtkm::cont::ArrayHandleUniformPointCoordinates& src,
  vtkm::IdComponent componentIndex,
  vtkm::CopyFlag allowCopy) const
{
  if (allowCopy != vtkm::CopyFlag::On)
  {
    throw vtkm::cont::ErrorBadValue(
      "Cannot extract component of ArrayHandleUniformPointCoordinates without copying. "
      "(However, the whole array does not need to be copied.)");
  }

  vtkm::Id3 dims = src.GetDimensions();
  vtkm::Vec3f origin = src.GetOrigin();
  vtkm::Vec3f spacing = src.GetSpacing();

  // A "slow" way to create the data, but the array is probably short. It would probably take
  // longer to schedule something on a device. (Can change that later if use cases change.)
  vtkm::cont::ArrayHandleBasic<vtkm::FloatDefault> componentArray;
  componentArray.Allocate(dims[componentIndex]);
  auto portal = componentArray.WritePortal();
  for (vtkm::Id i = 0; i < dims[componentIndex]; ++i)
  {
    portal.Set(i, origin[componentIndex] + (i * spacing[componentIndex]));
  }

  switch (componentIndex)
  {
    case 0:
      return vtkm::cont::ArrayHandleStride<vtkm::FloatDefault>(
        componentArray, src.GetNumberOfValues(), 1, 0, dims[0], 1);
    case 1:
      return vtkm::cont::ArrayHandleStride<vtkm::FloatDefault>(
        componentArray, src.GetNumberOfValues(), 1, 0, dims[1], dims[0]);
    case 2:
      return vtkm::cont::ArrayHandleStride<vtkm::FloatDefault>(
        componentArray, src.GetNumberOfValues(), 1, 0, 0, dims[0] * dims[1]);
    default:
      throw vtkm::cont::ErrorBadValue("Bad index given to ArrayExtractComponent.");
  }
}

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range>
ArrayRangeComputeImpl<vtkm::cont::StorageTagUniformPoints>::operator()(
  const vtkm::cont::ArrayHandleUniformPointCoordinates& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device) const
{
  if (maskArray.GetNumberOfValues() != 0)
  {
    return vtkm::cont::internal::ArrayRangeComputeGeneric(
      input, maskArray, computeFiniteRange, device);
  }

  vtkm::internal::ArrayPortalUniformPointCoordinates portal = input.ReadPortal();

  // In this portal we know that the min value is the first entry and the
  // max value is the last entry.
  vtkm::Vec3f minimum = portal.Get(0);
  vtkm::Vec3f maximum = portal.Get(portal.GetNumberOfValues() - 1);

  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray;
  rangeArray.Allocate(3);
  vtkm::cont::ArrayHandle<vtkm::Range>::WritePortalType outPortal = rangeArray.WritePortal();
  outPortal.Set(0, vtkm::Range(minimum[0], maximum[0]));
  outPortal.Set(1, vtkm::Range(minimum[1], maximum[1]));
  outPortal.Set(2, vtkm::Range(minimum[2], maximum[2]));

  return rangeArray;
}

} // namespace internal

}
} // namespace vtkm::cont
