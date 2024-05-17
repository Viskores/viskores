//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/scalar_topology/SelectTopVolumeContoursFilter.h>
#include <vtkm/filter/scalar_topology/internal/SelectTopVolumeContoursBlock.h>
#include <vtkm/filter/scalar_topology/internal/SelectTopVolumeContoursFunctor.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>


// vtkm includes
#include <vtkm/cont/Timer.h>

// DIY includes
// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{

VTKM_CONT vtkm::cont::DataSet SelectTopVolumeContoursFilter::DoExecute(const vtkm::cont::DataSet&)
{
  throw vtkm::cont::ErrorFilterExecution(
    "SelectTopVolumeContoursFilter expects PartitionedDataSet as input.");
}

VTKM_CONT vtkm::cont::PartitionedDataSet SelectTopVolumeContoursFilter::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();

  using SelectTopVolumeContoursBlock =
    vtkm::filter::scalar_topology::internal::SelectTopVolumeContoursBlock;
  vtkmdiy::Master branch_top_volume_master(comm,
                                           1,  // Use 1 thread, VTK-M will do the treading
                                           -1, // All blocks in memory
                                           0,  // No create function
                                           SelectTopVolumeContoursBlock::Destroy);

  auto firstDS = input.GetPartition(0);
  vtkm::Id3 firstPointDimensions, firstGlobalPointDimensions, firstGlobalPointIndexStart;
  firstDS.GetCellSet().CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    vtkm::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
    firstPointDimensions,
    firstGlobalPointDimensions,
    firstGlobalPointIndexStart);
  int numDims = firstGlobalPointDimensions[2] > 1 ? 3 : 2;
  auto vtkmBlocksPerDimensionRP = input.GetPartition(0)
                                    .GetField("vtkmBlocksPerDimension")
                                    .GetData()
                                    .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()
                                    .ReadPortal();

  int globalNumberOfBlocks = 1;

  for (vtkm::IdComponent d = 0; d < static_cast<vtkm::IdComponent>(numDims); ++d)
  {
    globalNumberOfBlocks *= static_cast<int>(vtkmBlocksPerDimensionRP.Get(d));
  }

  vtkmdiy::DynamicAssigner assigner(comm, size, globalNumberOfBlocks);
  for (vtkm::Id localBlockIndex = 0; localBlockIndex < input.GetNumberOfPartitions();
       ++localBlockIndex)
  {
    const vtkm::cont::DataSet& ds = input.GetPartition(localBlockIndex);
    int globalBlockId = static_cast<int>(
      vtkm::cont::ArrayGetValue(0,
                                ds.GetField("vtkmGlobalBlockId")
                                  .GetData()
                                  .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()));

    SelectTopVolumeContoursBlock* b =
      new SelectTopVolumeContoursBlock(localBlockIndex, globalBlockId);

    branch_top_volume_master.add(globalBlockId, b, new vtkmdiy::Link());
    assigner.set_rank(rank, globalBlockId);
  }

  vtkmdiy::fix_links(branch_top_volume_master, assigner);

  branch_top_volume_master.foreach (
    [&](SelectTopVolumeContoursBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      const auto& globalSize = firstGlobalPointDimensions;
      vtkm::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
      const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);

      b->SortBranchByVolume(ds, totalVolume);

      // copy the top volume branches into a smaller array
      // we skip index 0 because it must be the main branch (which has the highest volume)
      vtkm::Id nActualSavedBranches =
        std::min(this->nSavedBranches, b->SortedBranchByVolume.GetNumberOfValues() - 1);

      vtkm::worklet::contourtree_augmented::IdArrayType topVolumeBranch;
      vtkm::cont::Algorithm::CopySubRange(
        b->SortedBranchByVolume, 1, nActualSavedBranches, topVolumeBranch);

      auto branchRootGRId =
        ds.GetField("BranchRootGRId").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

      vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id>(
        branchRootGRId, topVolumeBranch, b->TopVolumeBranchRootGRId);

      vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id>(
        branchRootGRId, topVolumeBranch, b->TopVolumeBranchRootGRId);

      vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id>(
        b->BranchVolume, topVolumeBranch, b->TopVolumeBranchVolume);

      vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id>(
        b->BranchSaddleEpsilon, topVolumeBranch, b->TopVolumeBranchSaddleEpsilon);

      auto resolveArray = [&](const auto& inArray) {
        using InArrayHandleType = std::decay_t<decltype(inArray)>;
        InArrayHandleType topVolBranchSaddleIsoValue;
        vtkm::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
          inArray, topVolumeBranch, topVolBranchSaddleIsoValue);
        b->TopVolumeBranchSaddleIsoValue = topVolBranchSaddleIsoValue;
      };

      b->BranchSaddleIsoValue
        .CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(resolveArray);
    });

  // We apply all-to-all broadcast to collect the top nSavedBranches branches by volume
  vtkmdiy::all_to_all(
    branch_top_volume_master,
    assigner,
    vtkm::filter::scalar_topology::internal::SelectTopVolumeContoursFunctor(this->nSavedBranches));

  // For each block, we compute the get the extracted isosurface for every selected branch
  // storing format: key (branch ID) - Value (list of meshes in the isosurface)

  std::vector<vtkm::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());

  branch_top_volume_master.foreach (
    [&](SelectTopVolumeContoursBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      vtkm::cont::Field TopVolBranchGRIdField("TopVolumeBranchGlobalRegularIds",
                                              vtkm::cont::Field::Association::WholeDataSet,
                                              b->TopVolumeBranchRootGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchGRIdField);
      vtkm::cont::Field TopVolBranchVolumeField("TopVolumeBranchVolume",
                                                vtkm::cont::Field::Association::WholeDataSet,
                                                b->TopVolumeBranchVolume);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchVolumeField);
      vtkm::cont::Field TopVolBranchSaddleEpsilonField("TopVolumeBranchSaddleEpsilon",
                                                       vtkm::cont::Field::Association::WholeDataSet,
                                                       b->TopVolumeBranchSaddleEpsilon);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleEpsilonField);

      auto resolveArray = [&](const auto& inArray) {
        vtkm::cont::Field TopVolBranchSaddleIsoValueField(
          "TopVolumeBranchSaddleIsoValue", vtkm::cont::Field::Association::WholeDataSet, inArray);
        outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleIsoValueField);
      };
      b->TopVolumeBranchSaddleIsoValue
        .CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(resolveArray);
    });

  return vtkm::cont::PartitionedDataSet{ outputDataSets };
}

} // namespace scalar_topology
} // namespace filter
} // namespace vtkm
