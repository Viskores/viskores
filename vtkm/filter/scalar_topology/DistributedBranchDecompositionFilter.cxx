//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <vtkm/filter/scalar_topology/internal/BranchDecompositionBlock.h>
#include <vtkm/filter/scalar_topology/internal/ComputeDistributedBranchDecompositionFunctor.h>
#include <vtkm/filter/scalar_topology/internal/ExchangeBranchEndsFunctor.h>
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

// Constructor to  record information about spatial decomposition
// TODO/FIXME: Add this information to PartitionedDataSet, so that we do
// not need to pass it sperately (or check if it can already be derived from
// information stored in PartitionedDataSet)
VTKM_CONT DistributedBranchDecompositionFilter::DistributedBranchDecompositionFilter(
  vtkm::Id3,
  vtkm::Id3,
  const vtkm::cont::ArrayHandle<vtkm::Id3>&,
  const vtkm::cont::ArrayHandle<vtkm::Id3>&,
  const vtkm::cont::ArrayHandle<vtkm::Id3>&)
{
}

VTKM_CONT vtkm::cont::DataSet DistributedBranchDecompositionFilter::DoExecute(
  const vtkm::cont::DataSet&)
{
  throw vtkm::cont::ErrorFilterExecution(
    "DistributedBranchDecompositionFilter expects PartitionedDataSet as input.");
}

VTKM_CONT vtkm::cont::PartitionedDataSet DistributedBranchDecompositionFilter::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::Timer timer;
  timer.Start();
  std::stringstream timingsStream;

  // Set up DIY master
  // TODO/FIXME: A lot of the code to set up DIY is the same for this filter and
  // ContourTreeUniformDistributed. Consolidate? (Which is difficult to do as
  // multiple variables are set up with some subtle differences)
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();

  using BranchDecompositionBlock =
    vtkm::filter::scalar_topology::internal::BranchDecompositionBlock;
  vtkmdiy::Master branch_decomposition_master(comm,
                                              1,  // Use 1 thread, VTK-M will do the treading
                                              -1, // All blocks in memory
                                              0,  // No create function
                                              BranchDecompositionBlock::Destroy);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Master and Assigner (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Compute global ids (gids) for our local blocks
  // TODO/FIXME: Is there a better way to set this up?
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
    globalNumberOfBlocks *= diyDivisions[d];
    diyBounds.min[d] = 0;
    diyBounds.max[d] = static_cast<int>(firstGlobalPointDimensions[d]);
  }

  // Record time to compute the local block ids
  timingsStream << "    " << std::setw(60) << std::left
                << "Get DIY Information (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();


  // Initialize branch decomposition computation from data in PartitionedDataSet blocks
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

    BranchDecompositionBlock* newBlock =
      new BranchDecompositionBlock(localBlockIndex, globalBlockId, ds);
    // NOTE: Use dummy link to make DIY happy. The dummy link is never used, since all
    //       communication is via RegularDecomposer, which sets up its own links. No need
    //       to keep the pointer, as DIY will "own" it and delete it when no longer needed.
    // NOTE: Since we passed a "Destroy" function to DIY master, it will own the local data
    //       blocks and delete them when done.
    branch_decomposition_master.add(globalBlockId, newBlock, new vtkmdiy::Link());

    // Tell assigner that this block lives on this rank so that DIY can manage blocks
    assigner.set_rank(rank, globalBlockId);
  }

  // Log time to copy the data to the HyperSweepBlock data objects
  timingsStream << "    " << std::setw(60) << std::left << "Initialize Branch Decomposition Data"
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

  // TODO/FIXME: Check what happened here and possibly eliminate!
  for (vtkm::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
  }

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Decomposer and Assigner (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Fix the vtkmdiy links.
  vtkmdiy::fix_links(branch_decomposition_master, assigner);

  timingsStream << "    " << std::setw(60) << std::left << "Fix DIY Links (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // partners for merge over regular block grid
  vtkmdiy::RegularSwapPartners partners(
    decomposer, // domain decomposition
    2,          // radix of k-ary reduction.
    true        // contiguous: true=distance doubling, false=distance halving
  );

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Swap Partners (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Compute the initial volumes
  branch_decomposition_master.foreach (
    [&](BranchDecompositionBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      // Get intrinsic and dependent volume from data set
      const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
      vtkm::cont::ArrayHandle<vtkm::Id> intrinsicVolume =
        ds.GetField("IntrinsicVolume").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
      vtkm::cont::ArrayHandle<vtkm::Id> dependentVolume =
        ds.GetField("DependentVolume").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

      // Get global size and compute total volume from it
      const auto& globalSize = firstGlobalPointDimensions;
      vtkm::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];

      // Compute local best up and down paths by volume
      b->VolumetricBranchDecomposer.LocalBestUpDownByVolume(
        ds, intrinsicVolume, dependentVolume, totalVolume);

#ifdef DEBUG_PRINT
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Before reduction");
      {
        std::stringstream rs;
        vtkm::worklet::contourtree_augmented::PrintHeader(
          b->VolumetricBranchDecomposer.BestUpSupernode.GetNumberOfValues(), rs);
        vtkm::worklet::contourtree_augmented::PrintIndices(
          "BestUpSupernode", b->VolumetricBranchDecomposer.BestUpSupernode, -1, rs);
        vtkm::worklet::contourtree_augmented::PrintIndices(
          "BestDownSupernode", b->VolumetricBranchDecomposer.BestDownSupernode, -1, rs);
        vtkm::worklet::contourtree_augmented::PrintIndices(
          "BestUpVolume", b->VolumetricBranchDecomposer.BestUpVolume, -1, rs);
        vtkm::worklet::contourtree_augmented::PrintIndices(
          "BestDownVolume", b->VolumetricBranchDecomposer.BestDownVolume, -1, rs);
        VTKM_LOG_S(vtkm::cont::LogLevel::Info, rs.str());
      }
#endif
    });

  timingsStream << "    " << std::setw(60) << std::left << "LocalBestUpDownByVolume"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Reduce
  // partners for merge over regular block grid
  vtkmdiy::reduce(
    branch_decomposition_master,
    assigner,
    partners,
    vtkm::filter::scalar_topology::internal::ComputeDistributedBranchDecompositionFunctor{});

  timingsStream << "    " << std::setw(60) << std::left
                << "Exchanging best up/down supernode and volume"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  branch_decomposition_master.foreach (
    [&](BranchDecompositionBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
      b->VolumetricBranchDecomposer.CollapseBranches(ds, b->BranchRoots);
    });

  timingsStream << "    " << std::setw(38) << std::left << "CollapseBranches"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  //// Branch decompisition stored in branch root array b->BranchRoots
  branch_decomposition_master.foreach (
    [&](BranchDecompositionBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
  //// STEP 1: Find ends of branches locally
  //// STEP 1A: Find upper end of branch locally
  //// Segmented sort by branch ID of value of upper node of superarc
  //// Sort superarcs by value value of upper node, segmenting by branchID
  //// Upper node determined using ascending flag of superarc array
  //// NOTE: Superarc array is stored in b->HierarchicalContourTreeDataSet
  //// if ascending flag is NOT set, upper node is the source node of the superarc, whose
  //// supernode ID is guaranteed to be the same as the ID of the superarc
  //// if ascending flag is set, upper node is the target node of the superarc, which is stored
  //// in the superarc array but maskIndex must be called to strip out flags
  //// Create index array with IDs of all superarcs:
  ////    * Size is Supernodes.Size()-1 or Superarcs.Size()-1 because of last node as NULL superarc
  ////    * Fill vtk-m equivalent of std::iota
  //// Segmented sort of the "superarcs" array sort by three keys:
  ////    (1) branchID (most senior superarc),
  ////    (2) data value
  ////    (3) global regular id (for simulation of simplicity)
  //// Find highest vertex for branch (i.e., before branchID increases), special case for
  //// end of array.
  //
  //// Based on level of the block, the attachment points (if not the highest level) or the root of the contour tree
  //// their Superarcs should always be NO_SUCH_ELEMENT (NSE)
  //// If attachment points, their Superparents should hold the superarc ID they attach to
  //// STEP 1B: Find lower end of branch locally
  ////    Inverse to STEP 1A
  //
  //// STEP 1C: Compress out duplicate branch IDs
  ////   * Temporary array "knownBranches" with size of superarcs array, initialize to NO_SUCH_ELEMENT
  ////   * Every highest vertex we find in STEP 1A has a branch ID, use that ID to set knownBranches[bID] = bID;
  //// . * Remove/compress out NO_SUCH_ELEMENT entries
  //// . * Array now is a list of all known (to the block) branches
  //
  //// STEP 2: Look up (and add) global regular ID, value, and terminal volume both intrinsic and dependent
  //// Target: get the information to explicitly extract the branch
  //// NOTE: Both STEP 1 and STEP 2 are implemented in b->VolumetricBranchDecomposer.CollectBranches()
  //// =================================================================
  //// Pipeline:
  //// Each block now has a list of all the branchRoot IDs;
  //// convert it into a list of global regular ids for each branch;
  //// obtain the value based on the local regular id;
  //// dependent volume is indexed by the superarc id; however, it's the superarc id of the last superarc on the branch
  //// and we don't know the direction of the superarc
  //// As a result, the top supernode can either be the source or the destination of the superarc
  //// and, the dependent volume could be at either end
  //// "IsAscending(superarc)" tells the direction of the superarc, and consequently the direction of the dependent volume
  //// Therefore, we treat the highest end and lowest end as the SUPERARC rather than nodes due to direction information
  //// Moreover, for all branches other than the senior most, either the top end or the bottom end is a leaf, and the other end is the inner end (saddle)
  //// Leaves can be detected because the dependent weight is always totalVolume(mesh)-1;
  //// Senior branch will have leaves on both ends.
#ifdef DEBUG_PRINT
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "CollectBranches for local block " << b->GlobalBlockId << std::endl);
#endif
      const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
      b->VolumetricBranchDecomposer.CollectBranches(ds, b->BranchRoots);
    });

  // Now we have collected the branches, we do a global reduction to exchance branch end information
  // across all compute ranks
  vtkmdiy::reduce(branch_decomposition_master,
                  assigner,
                  partners,
                  vtkm::filter::scalar_topology::internal::ExchangeBranchEndsFunctor{});

  std::vector<vtkm::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());
  // Copy input data set to output
  // TODO/FIXME: Should we really do this? Or just output branchRoots
  // and let the application deal with two ParitionedDataSet objects
  // if it also needs access to the other contour tree data
  for (vtkm::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
  {
    outputDataSets[ds_no] = input.GetPartition(ds_no);
  }

  branch_decomposition_master.foreach (
    [&](BranchDecompositionBlock* b, const vtkmdiy::Master::ProxyWithLink&) {
      vtkm::cont::Field branchRootField(
        "BranchRoots", vtkm::cont::Field::Association::WholeDataSet, b->BranchRoots);
      outputDataSets[b->LocalBlockNo].AddField(branchRootField);

      // Store the upper end and lower end global regular IDs of branches in the output
      vtkm::cont::Field UpperEndGRIdField("UpperEndGlobalRegularIds",
                                          vtkm::cont::Field::Association::WholeDataSet,
                                          b->VolumetricBranchDecomposer.UpperEndGRId);
      outputDataSets[b->LocalBlockNo].AddField(UpperEndGRIdField);
      vtkm::cont::Field LowerEndGRIdField("LowerEndGlobalRegularIds",
                                          vtkm::cont::Field::Association::WholeDataSet,
                                          b->VolumetricBranchDecomposer.LowerEndGRId);
      outputDataSets[b->LocalBlockNo].AddField(LowerEndGRIdField);

      vtkm::cont::Field UpperEndIntrinsicVolume(
        "UpperEndIntrinsicVolume",
        vtkm::cont::Field::Association::WholeDataSet,
        b->VolumetricBranchDecomposer.UpperEndIntrinsicVolume);
      outputDataSets[b->LocalBlockNo].AddField(UpperEndIntrinsicVolume);
      vtkm::cont::Field UpperEndDependentVolume(
        "UpperEndDependentVolume",
        vtkm::cont::Field::Association::WholeDataSet,
        b->VolumetricBranchDecomposer.UpperEndDependentVolume);
      outputDataSets[b->LocalBlockNo].AddField(UpperEndDependentVolume);
      vtkm::cont::Field LowerEndIntrinsicVolume(
        "LowerEndIntrinsicVolume",
        vtkm::cont::Field::Association::WholeDataSet,
        b->VolumetricBranchDecomposer.LowerEndIntrinsicVolume);
      outputDataSets[b->LocalBlockNo].AddField(LowerEndIntrinsicVolume);
      vtkm::cont::Field LowerEndDependentVolume(
        "LowerEndDependentVolume",
        vtkm::cont::Field::Association::WholeDataSet,
        b->VolumetricBranchDecomposer.LowerEndDependentVolume);
      outputDataSets[b->LocalBlockNo].AddField(LowerEndDependentVolume);
      vtkm::cont::Field LowerEndSuperarcId("LowerEndSuperarcId",
                                           vtkm::cont::Field::Association::WholeDataSet,
                                           b->VolumetricBranchDecomposer.LowerEndSuperarcId);
      outputDataSets[b->LocalBlockNo].AddField(LowerEndSuperarcId);
      vtkm::cont::Field UpperEndSuperarcId("UpperEndSuperarcId",
                                           vtkm::cont::Field::Association::WholeDataSet,
                                           b->VolumetricBranchDecomposer.UpperEndSuperarcId);
      outputDataSets[b->LocalBlockNo].AddField(UpperEndSuperarcId);
      vtkm::cont::Field LowerEndValue("LowerEndValue",
                                      vtkm::cont::Field::Association::WholeDataSet,
                                      b->VolumetricBranchDecomposer.LowerEndValue);
      outputDataSets[b->LocalBlockNo].AddField(LowerEndValue);
      vtkm::cont::Field UpperEndValue("UpperEndValue",
                                      vtkm::cont::Field::Association::WholeDataSet,
                                      b->VolumetricBranchDecomposer.UpperEndValue);
      outputDataSets[b->LocalBlockNo].AddField(UpperEndValue);
      vtkm::cont::Field BranchRoot("BranchRoot",
                                   vtkm::cont::Field::Association::WholeDataSet,
                                   b->VolumetricBranchDecomposer.BranchRoot);
      outputDataSets[b->LocalBlockNo].AddField(BranchRoot);
      vtkm::cont::Field BranchRootGRId("BranchRootGRId",
                                       vtkm::cont::Field::Association::WholeDataSet,
                                       b->VolumetricBranchDecomposer.BranchRootGRId);
      outputDataSets[b->LocalBlockNo].AddField(BranchRootGRId);
    });

  timingsStream << "    " << std::setw(38) << std::left
                << "Creating Branch Decomposition Output Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;

  VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
             std::endl
               << "-----------  DoExecutePartitions Timings ------------" << std::endl
               << timingsStream.str());

  return vtkm::cont::PartitionedDataSet{ outputDataSets };
}

} // namespace scalar_topology
} // namespace filter
} // namespace vtkm
