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
#include <vtkm/filter/scalar_topology/SelectTopVolumeBranchesFilter.h>
#include <vtkm/filter/scalar_topology/internal/SelectTopVolumeBranchesBlock.h>
#include <vtkm/filter/scalar_topology/internal/SelectTopVolumeBranchesFunctor.h>
#include <vtkm/filter/scalar_topology/internal/UpdateParentBranchFunctor.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>

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

VTKM_CONT vtkm::cont::DataSet SelectTopVolumeBranchesFilter::DoExecute(const vtkm::cont::DataSet&)
{
  throw vtkm::cont::ErrorFilterExecution(
    "SelectTopVolumeBranchesFilter expects PartitionedDataSet as input.");
}

VTKM_CONT vtkm::cont::PartitionedDataSet SelectTopVolumeBranchesFilter::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::Timer timer;
  timer.Start();
  std::stringstream timingsStream;

  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();


  using SelectTopVolumeBranchesBlock =
    vtkm::filter::scalar_topology::internal::SelectTopVolumeBranchesBlock;
  vtkmdiy::Master branch_top_volume_master(comm,
                                           1,  // Use 1 thread, VTK-M will do the treading
                                           -1, // All blocks in memory
                                           0,  // No create function
                                           SelectTopVolumeBranchesBlock::Destroy);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Master and Assigner (Branch Selection)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

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

  // ... compute division vector for global domain
  using RegularDecomposer = vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds>;
  RegularDecomposer::DivisionsVector diyDivisions(numDims);
  vtkmdiy::DiscreteBounds diyBounds(numDims);
  int globalNumberOfBlocks = 1;

  for (vtkm::IdComponent d = 0; d < static_cast<vtkm::IdComponent>(numDims); ++d)
  {
    diyDivisions[d] = static_cast<int>(vtkmBlocksPerDimensionRP.Get(d));
    globalNumberOfBlocks *= static_cast<int>(vtkmBlocksPerDimensionRP.Get(d));
    diyBounds.min[d] = 0;
    diyBounds.max[d] = static_cast<int>(firstGlobalPointDimensions[d]);
  }

  // Record time to compute the local block ids

  timingsStream << "    " << std::setw(60) << std::left << "Get DIY Information (Branch Selection)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

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

    SelectTopVolumeBranchesBlock* b =
      new SelectTopVolumeBranchesBlock(localBlockIndex, globalBlockId);

    branch_top_volume_master.add(globalBlockId, b, new vtkmdiy::Link());
    assigner.set_rank(rank, globalBlockId);
  }

  // Log time to copy the data to the block data objects
  timingsStream << "    " << std::setw(60) << std::left << "Initialize Branch Selection Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Set up DIY for binary reduction
  RegularDecomposer::BoolVector shareFace(3, true);
  RegularDecomposer::BoolVector wrap(3, false);
  RegularDecomposer::CoordinateVector ghosts(3, 1);
  RegularDecomposer decomposer(numDims,
                               diyBounds,
                               static_cast<int>(globalNumberOfBlocks),
                               shareFace,
                               wrap,
                               ghosts,
                               diyDivisions);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Decomposer and Assigner (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Fix the vtkmdiy links.
  vtkmdiy::fix_links(branch_top_volume_master, assigner);

  timingsStream << "    " << std::setw(60) << std::left << "Fix DIY Links (Branch Selection)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // partners for merge over regular block grid
  vtkmdiy::RegularSwapPartners partners(
    decomposer, // domain decomposition
    2,          // radix of k-ary reduction.
    true        // contiguous: true=distance doubling, false=distance halving
  );

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Swap Partners (Branch Selection)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // compute the branch volume, and select the top branch by volume locally
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      using vtkm::worklet::contourtree_augmented::IdArrayType;
      const auto& globalSize = firstGlobalPointDimensions;
      vtkm::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
      const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);

      // compute the volume of branches
      b->SortBranchByVolume(ds, totalVolume);
      // select the top branch by volume
      b->SelectLocalTopVolumeBranches(ds, this->GetSavedBranches());
    });

  timingsStream << "    " << std::setw(60) << std::left << "SelectBranchByVolume"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // We apply block reduction to collect the top NumSavedBranches branches by volume
  vtkmdiy::reduce(branch_top_volume_master,
                  assigner,
                  partners,
                  vtkm::filter::scalar_topology::internal::SelectTopVolumeBranchesFunctor(
                    this->NumSavedBranches, this->TimingsLogLevel));

  timingsStream << "    " << std::setw(60) << std::left << "SelectGlobalTopVolumeBranches"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // before computing the hierarchy of selected branches, we exclude selected branches
  // with volume <= presimplifyThreshold
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      this->SetSavedBranches(b->ExcludeTopVolumeBranchByThreshold(this->GetPresimplifyThreshold()));
    });

  // if we do not have any saved branches,
  //   case 1. didn't specify nBranches correctly, and/or
  //   case 2. over pre-simplified,
  // then we terminate the function prematurely.
  if (this->NumSavedBranches <= 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "No branch is remaining!\n"
               "Check the presimplification level or the number of branches to save.");
    std::vector<vtkm::cont::DataSet> emptyDataSets(input.GetNumberOfPartitions());
    return vtkm::cont::PartitionedDataSet{ emptyDataSets };
  }

  // we compute the hierarchy of selected branches adding the root branch for each block
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
      b->ComputeTopVolumeBranchHierarchy(ds);
    });

  timingsStream << "    " << std::setw(60) << std::left << "ComputeTopVolumeBranchHierarchy"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // We apply block reduction to update
  //   1. the global branch hierarchy
  //   2. the outer-most saddle isovalue on all parent branches
  vtkmdiy::reduce(
    branch_top_volume_master,
    assigner,
    partners,
    vtkm::filter::scalar_topology::internal::UpdateParentBranchFunctor(this->TimingsLogLevel));

  timingsStream << "    " << std::setw(60) << std::left << "Update Parent Branch Information"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // The next step is to extract contours.
  // However, we use a separate filter to do it.
  // This is because we want to utilize the existing Contour filter in VTK-m,
  // but the work is not trivial and need more discussion (e.g., implicit mesh triangulation)

  // Create output dataset
  std::vector<vtkm::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());
  // Copy input data set to output
  // This will make the output dataset pretty large.
  // Unfortunately, this step seems to be inevitable,
  // because searching for the superarc of cells requires information of the contour tree
  for (vtkm::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
  {
    outputDataSets[ds_no] = input.GetPartition(ds_no);
  }

  // we need to send everything that contour extraction needs to the output dataset
  branch_top_volume_master.foreach ([&](SelectTopVolumeBranchesBlock* b,
                                        const vtkmdiy::Master::ProxyWithLink&) {
    vtkm::cont::Field BranchVolumeField(
      "BranchVolume", vtkm::cont::Field::Association::WholeDataSet, b->TopVolumeData.BranchVolume);
    outputDataSets[b->LocalBlockNo].AddField(BranchVolumeField);
    vtkm::cont::Field BranchSaddleEpsilonField("BranchSaddleEpsilon",
                                               vtkm::cont::Field::Association::WholeDataSet,
                                               b->TopVolumeData.BranchSaddleEpsilon);
    outputDataSets[b->LocalBlockNo].AddField(BranchSaddleEpsilonField);
    vtkm::cont::Field TopVolBranchUpperEndField("TopVolumeBranchUpperEnd",
                                                vtkm::cont::Field::Association::WholeDataSet,
                                                b->TopVolumeData.TopVolumeBranchUpperEndGRId);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchUpperEndField);
    vtkm::cont::Field TopVolBranchLowerEndField("TopVolumeBranchLowerEnd",
                                                vtkm::cont::Field::Association::WholeDataSet,
                                                b->TopVolumeData.TopVolumeBranchLowerEndGRId);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchLowerEndField);
    vtkm::cont::Field TopVolumeBranchGRIdsField("TopVolumeBranchGlobalRegularIds",
                                                vtkm::cont::Field::Association::WholeDataSet,
                                                b->TopVolumeData.TopVolumeBranchRootGRId);
    outputDataSets[b->LocalBlockNo].AddField(TopVolumeBranchGRIdsField);
    vtkm::cont::Field TopVolBranchVolumeField("TopVolumeBranchVolume",
                                              vtkm::cont::Field::Association::WholeDataSet,
                                              b->TopVolumeData.TopVolumeBranchVolume);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchVolumeField);
    vtkm::cont::Field TopVolBranchSaddleEpsilonField("TopVolumeBranchSaddleEpsilon",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.TopVolumeBranchSaddleEpsilon);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleEpsilonField);
    vtkm::cont::Field TopVolBranchSaddleIsoValueField(
      "TopVolumeBranchSaddleIsoValue",
      vtkm::cont::Field::Association::WholeDataSet,
      b->TopVolumeData.TopVolumeBranchSaddleIsoValue);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleIsoValueField);

    // additional data for isosurface extraction.
    // Most of them are intermediate arrays and should not be parts of the actual output.
    // this->TopVolumeData.TopVolBranchKnownByBlockStencil
    vtkm::cont::Field TopVolBranchKnownByBlockStencilField(
      "TopVolumeBranchKnownByBlockStencil",
      vtkm::cont::Field::Association::WholeDataSet,
      b->TopVolumeData.TopVolBranchKnownByBlockStencil);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchKnownByBlockStencilField);
    // this->TopVolumeData.TopVolBranchInfoActualIndex
    vtkm::cont::Field TopVolBranchInfoActualIndexField(
      "TopVolumeBranchInformationIndex",
      vtkm::cont::Field::Association::WholeDataSet,
      b->TopVolumeData.TopVolBranchInfoActualIndex);
    outputDataSets[b->LocalBlockNo].AddField(TopVolBranchInfoActualIndexField);
    // this->TopVolumeData.IsParentBranch
    vtkm::cont::Field IsParentBranchField("IsParentBranch",
                                          vtkm::cont::Field::Association::WholeDataSet,
                                          b->TopVolumeData.IsParentBranch);
    outputDataSets[b->LocalBlockNo].AddField(IsParentBranchField);
    // this->TopVolumeData.ExtraMaximaBranchLowerEnd
    vtkm::cont::Field ExtraMaximaBranchLowerEndField("ExtraMaximaBranchLowerEnd",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.ExtraMaximaBranchLowerEnd);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchLowerEndField);
    // this->TopVolumeData.ExtraMaximaBranchUpperEnd
    vtkm::cont::Field ExtraMaximaBranchUpperEndField("ExtraMaximaBranchUpperEnd",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.ExtraMaximaBranchUpperEnd);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchUpperEndField);
    // this->TopVolumeData.ExtraMaximaBranchOrder
    vtkm::cont::Field ExtraMaximaBranchOrderField("ExtraMaximaBranchOrder",
                                                  vtkm::cont::Field::Association::WholeDataSet,
                                                  b->TopVolumeData.ExtraMaximaBranchOrder);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchOrderField);
    // this->TopVolumeData.ExtraMaximaBranchSaddleGRId
    vtkm::cont::Field ExtraMaximaBranchSaddleGRIdField(
      "ExtraMaximaBranchSaddleGRId",
      vtkm::cont::Field::Association::WholeDataSet,
      b->TopVolumeData.ExtraMaximaBranchSaddleGRId);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchSaddleGRIdField);
    // this->TopVolumeData.ExtraMaximaBranchIsoValue
    vtkm::cont::Field ExtraMaximaBranchIsoValueField("ExtraMaximaBranchIsoValue",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.ExtraMaximaBranchIsoValue);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchIsoValueField);
    // this->TopVolumeData.ExtraMinimaBranchLowerEnd
    vtkm::cont::Field ExtraMinimaBranchLowerEndField("ExtraMinimaBranchLowerEnd",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.ExtraMinimaBranchLowerEnd);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchLowerEndField);
    // this->TopVolumeData.ExtraMinimaBranchUpperEnd
    vtkm::cont::Field ExtraMinimaBranchUpperEndField("ExtraMinimaBranchUpperEnd",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.ExtraMinimaBranchUpperEnd);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchUpperEndField);
    // this->TopVolumeData.ExtraMinimaBranchOrder
    vtkm::cont::Field ExtraMinimaBranchOrderField("ExtraMinimaBranchOrder",
                                                  vtkm::cont::Field::Association::WholeDataSet,
                                                  b->TopVolumeData.ExtraMinimaBranchOrder);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchOrderField);
    // this->TopVolumeData.ExtraMinimaBranchSaddleGRId
    vtkm::cont::Field ExtraMinimaBranchSaddleGRIdField(
      "ExtraMinimaBranchSaddleGRId",
      vtkm::cont::Field::Association::WholeDataSet,
      b->TopVolumeData.ExtraMinimaBranchSaddleGRId);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchSaddleGRIdField);
    // this->TopVolumeData.ExtraMinimaBranchIsoValue
    vtkm::cont::Field ExtraMinimaBranchIsoValueField("ExtraMinimaBranchIsoValue",
                                                     vtkm::cont::Field::Association::WholeDataSet,
                                                     b->TopVolumeData.ExtraMinimaBranchIsoValue);
    outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchIsoValueField);
  });

  timingsStream << "    " << std::setw(38) << std::left << "Creating Branch Selection Output Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;

  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "-----------  DoExecutePartitions Timings ------------" << std::endl
               << timingsStream.str());

  return vtkm::cont::PartitionedDataSet{ outputDataSets };
}

} // namespace scalar_topology
} // namespace filter
} // namespace vtkm
