//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_flow_internal_ParticleBlockIds_h
#define viskores_filter_flow_internal_ParticleBlockIds_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{
namespace detail
{

class CountParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particles, ExecObject locator, FieldOut count);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ParticleType, typename LocatorType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const LocatorType& locator,
                                viskores::Id& count) const
  {
    count = locator.CountAllCells(particle.GetPosition());
  }
};

class FindParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particles, ExecObject locator, FieldOut blockIds);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ParticleType, typename LocatorType, typename BlockIdVecType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const LocatorType& locator,
                                BlockIdVecType& blockIds) const
  {
    locator.FindAllCellIds(particle.GetPosition(), blockIds);
  }
};

} // namespace detail

template <typename ParticleType, typename StorageType>
VISKORES_CONT auto FindParticleBlockIds(
  const viskores::cont::ArrayHandle<ParticleType, StorageType>& particles,
  const viskores::filter::flow::internal::BoundsMap& boundsMap)
{
  // A particle can be inside multiple overlapping blocks. Count first so the
  // variable-length block id groups can be allocated exactly, then fill them.
  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<viskores::Id> blockCounts;
  invoker(
    detail::CountParticleBlockIdsWorklet{}, particles, boundsMap.GetLocator(), blockCounts);

  viskores::Id totalBlockIds = 0;
  auto offsets = viskores::cont::ConvertNumComponentsToOffsets(blockCounts, totalBlockIds);

  viskores::cont::ArrayHandle<viskores::Id> blockIds;
  blockIds.AllocateAndFill(totalBlockIds, viskores::Id{ -1 });
  auto groupedBlockIds = viskores::cont::make_ArrayHandleGroupVecVariable(blockIds, offsets);

  invoker(
    detail::FindParticleBlockIdsWorklet{}, particles, boundsMap.GetLocator(), groupedBlockIds);
  return groupedBlockIds;
}

}
}
}
} // namespace viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_ParticleBlockIds_h
