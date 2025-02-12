//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/scalar_topology/SelectTopVolumeContoursFilter.h>
#include <viskores/filter/scalar_topology/internal/SelectTopVolumeContoursBlock.h>
#include <viskores/filter/scalar_topology/internal/SelectTopVolumeContoursFunctor.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>


// viskores includes
#include <viskores/cont/Timer.h>

// DIY includes
// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/Configure.h>
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

VISKORES_CONT viskores::cont::DataSet SelectTopVolumeContoursFilter::DoExecute(const viskores::cont::DataSet&)
{
  throw viskores::cont::ErrorFilterExecution(
    "SelectTopVolumeContoursFilter expects PartitionedDataSet as input.");
}

VISKORES_CONT viskores::cont::PartitionedDataSet SelectTopVolumeContoursFilter::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();

  using SelectTopVolumeContoursBlock =
    viskores::filter::scalar_topology::internal::SelectTopVolumeContoursBlock;
  viskoresdiy::Master branch_top_volume_master(comm,
                                           1,  // Use 1 thread, Viskores will do the treading
                                           -1, // All blocks in memory
                                           0,  // No create function
                                           SelectTopVolumeContoursBlock::Destroy);

  auto firstDS = input.GetPartition(0);
  viskores::Id3 firstPointDimensions, firstGlobalPointDimensions, firstGlobalPointIndexStart;
  firstDS.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
    firstPointDimensions,
    firstGlobalPointDimensions,
    firstGlobalPointIndexStart);
  int numDims = firstGlobalPointDimensions[2] > 1 ? 3 : 2;
  auto viskoresBlocksPerDimensionRP = input.GetPartition(0)
                                    .GetField("viskoresBlocksPerDimension")
                                    .GetData()
                                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                    .ReadPortal();

  int globalNumberOfBlocks = 1;

  for (viskores::IdComponent d = 0; d < static_cast<viskores::IdComponent>(numDims); ++d)
  {
    globalNumberOfBlocks *= static_cast<int>(viskoresBlocksPerDimensionRP.Get(d));
  }

  viskoresdiy::DynamicAssigner assigner(comm, size, globalNumberOfBlocks);
  for (viskores::Id localBlockIndex = 0; localBlockIndex < input.GetNumberOfPartitions();
       ++localBlockIndex)
  {
    const viskores::cont::DataSet& ds = input.GetPartition(localBlockIndex);
    int globalBlockId = static_cast<int>(
      viskores::cont::ArrayGetValue(0,
                                ds.GetField("viskoresGlobalBlockId")
                                  .GetData()
                                  .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()));

    SelectTopVolumeContoursBlock* b =
      new SelectTopVolumeContoursBlock(localBlockIndex, globalBlockId);

    branch_top_volume_master.add(globalBlockId, b, new viskoresdiy::Link());
    assigner.set_rank(rank, globalBlockId);
  }

  viskoresdiy::fix_links(branch_top_volume_master, assigner);

  branch_top_volume_master.foreach (
    [&](SelectTopVolumeContoursBlock* b, const viskoresdiy::Master::ProxyWithLink&) {
      const auto& globalSize = firstGlobalPointDimensions;
      viskores::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
      const viskores::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);

      b->SortBranchByVolume(ds, totalVolume);

      // copy the top volume branches into a smaller array
      // we skip index 0 because it must be the main branch (which has the highest volume)
      viskores::Id nActualSavedBranches =
        std::min(this->nSavedBranches, b->SortedBranchByVolume.GetNumberOfValues() - 1);

      viskores::worklet::contourtree_augmented::IdArrayType topVolumeBranch;
      viskores::cont::Algorithm::CopySubRange(
        b->SortedBranchByVolume, 1, nActualSavedBranches, topVolumeBranch);

      auto branchRootGRId =
        ds.GetField("BranchRootGRId").GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

      auto upperEndGRId = ds.GetField("UpperEndGlobalRegularIds")
                            .GetData()
                            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

      auto lowerEndGRId = ds.GetField("LowerEndGlobalRegularIds")
                            .GetData()
                            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
        branchRootGRId, topVolumeBranch, b->TopVolumeBranchRootGRId);

      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
        branchRootGRId, topVolumeBranch, b->TopVolumeBranchRootGRId);

      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
        b->BranchVolume, topVolumeBranch, b->TopVolumeBranchVolume);

      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
        b->BranchSaddleEpsilon, topVolumeBranch, b->TopVolumeBranchSaddleEpsilon);

      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
        upperEndGRId, topVolumeBranch, b->TopVolumeBranchUpperEndGRId);

      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
        lowerEndGRId, topVolumeBranch, b->TopVolumeBranchLowerEndGRId);

      auto resolveArray = [&](const auto& inArray) {
        using InArrayHandleType = std::decay_t<decltype(inArray)>;
        InArrayHandleType topVolBranchSaddleIsoValue;
        viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
          inArray, topVolumeBranch, topVolBranchSaddleIsoValue);
        b->TopVolumeBranchSaddleIsoValue = topVolBranchSaddleIsoValue;
      };

      b->BranchSaddleIsoValue
        .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(resolveArray);
    });

  // We apply all-to-all broadcast to collect the top nSavedBranches branches by volume
  viskoresdiy::all_to_all(
    branch_top_volume_master,
    assigner,
    viskores::filter::scalar_topology::internal::SelectTopVolumeContoursFunctor(this->nSavedBranches));

  // For each block, we compute the get the extracted isosurface for every selected branch
  // storing format: key (branch ID) - Value (list of meshes in the isosurface)

  std::vector<viskores::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());

  branch_top_volume_master.foreach (
    [&](SelectTopVolumeContoursBlock* b, const viskoresdiy::Master::ProxyWithLink&) {
      viskores::cont::Field TopVolBranchUpperEndField("TopVolumeBranchUpperEnd",
                                                  viskores::cont::Field::Association::WholeDataSet,
                                                  b->TopVolumeBranchUpperEndGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchUpperEndField);
      viskores::cont::Field TopVolBranchLowerEndField("TopVolumeBranchLowerEnd",
                                                  viskores::cont::Field::Association::WholeDataSet,
                                                  b->TopVolumeBranchLowerEndGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchLowerEndField);
      viskores::cont::Field TopVolBranchGRIdField("TopVolumeBranchGlobalRegularIds",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              b->TopVolumeBranchRootGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchGRIdField);
      viskores::cont::Field TopVolBranchVolumeField("TopVolumeBranchVolume",
                                                viskores::cont::Field::Association::WholeDataSet,
                                                b->TopVolumeBranchVolume);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchVolumeField);
      viskores::cont::Field TopVolBranchSaddleEpsilonField("TopVolumeBranchSaddleEpsilon",
                                                       viskores::cont::Field::Association::WholeDataSet,
                                                       b->TopVolumeBranchSaddleEpsilon);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleEpsilonField);

      auto resolveArray = [&](const auto& inArray) {
        viskores::cont::Field TopVolBranchSaddleIsoValueField(
          "TopVolumeBranchSaddleIsoValue", viskores::cont::Field::Association::WholeDataSet, inArray);
        outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleIsoValueField);
      };
      b->TopVolumeBranchSaddleIsoValue
        .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(resolveArray);
    });

  return viskores::cont::PartitionedDataSet{ outputDataSets };
}

} // namespace scalar_topology
} // namespace filter
} // namespace viskores
