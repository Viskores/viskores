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
#include <vtkm/filter/scalar_topology/ExtractTopVolumeContoursFilter.h>
#include <vtkm/filter/scalar_topology/internal/ExtractTopVolumeContoursBlock.h>
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

VTKM_CONT vtkm::cont::DataSet ExtractTopVolumeContoursFilter::DoExecute(const vtkm::cont::DataSet&)
{
  throw vtkm::cont::ErrorFilterExecution(
    "ExtractTopVolumeContoursFilter expects PartitionedDataSet as input.");
}

VTKM_CONT vtkm::cont::PartitionedDataSet ExtractTopVolumeContoursFilter::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::Timer timer;
  timer.Start();
  std::stringstream timingsStream;

  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();


  using ExtractTopVolumeContoursBlock =
    vtkm::filter::scalar_topology::internal::ExtractTopVolumeContoursBlock;
  vtkmdiy::Master branch_top_volume_master(comm,
                                           1,  // Use 1 thread, VTK-M will do the treading
                                           -1, // All blocks in memory
                                           0,  // No create function
                                           ExtractTopVolumeContoursBlock::Destroy);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Master and Assigner (Contour Extraction)"
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
  int globalNumberOfBlocks = 1;

  for (vtkm::IdComponent d = 0; d < static_cast<vtkm::IdComponent>(numDims); ++d)
  {
    globalNumberOfBlocks *= static_cast<int>(vtkmBlocksPerDimensionRP.Get(d));
  }

  // Record time to compute the local block ids
  timingsStream << "    " << std::setw(60) << std::left
                << "Get DIY Information (Contour Extraction)"
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

    ExtractTopVolumeContoursBlock* b =
      new ExtractTopVolumeContoursBlock(localBlockIndex, globalBlockId);

    branch_top_volume_master.add(globalBlockId, b, new vtkmdiy::Link());
    assigner.set_rank(rank, globalBlockId);
  }

  // Log time to copy the data to the block data objects
  timingsStream << "    " << std::setw(60) << std::left << "Initialize Contour Extraction Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Assigner (Contour Extraction)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Fix the vtkmdiy links.
  vtkmdiy::fix_links(branch_top_volume_master, assigner);

  timingsStream << "    " << std::setw(60) << std::left << "Fix DIY Links (Contour Extraction)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // We compute everything we need for contour extraction and put them in the output dataset.
  branch_top_volume_master.foreach ([&](ExtractTopVolumeContoursBlock* b,
                                        const vtkmdiy::Master::ProxyWithLink&) {
    const vtkm::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
    b->ExtractIsosurfaceOnSelectedBranch(
      ds, this->GetMarchingCubes(), this->GetShiftIsovalueByEpsilon(), this->GetTimingsLogLevel());
  });

  timingsStream << "    " << std::setw(60) << std::left << "Draw Contours By Branches"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  std::vector<vtkm::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());
  // we need to send everything that contour extraction needs to the output dataset
  branch_top_volume_master.foreach ([&](ExtractTopVolumeContoursBlock* b,
                                        const vtkmdiy::Master::ProxyWithLink&) {
    vtkm::cont::Field IsosurfaceEdgeFromField(
      "IsosurfaceEdgesFrom", vtkm::cont::Field::Association::WholeDataSet, b->IsosurfaceEdgesFrom);
    outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeFromField);
    vtkm::cont::Field IsosurfaceEdgeToField(
      "IsosurfaceEdgesTo", vtkm::cont::Field::Association::WholeDataSet, b->IsosurfaceEdgesTo);
    outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeToField);
    vtkm::cont::Field IsosurfaceEdgeLabelField("IsosurfaceEdgesLabels",
                                               vtkm::cont::Field::Association::WholeDataSet,
                                               b->IsosurfaceEdgesLabels);
    outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeLabelField);

    vtkm::cont::Field IsosurfaceEdgeOffsetField("IsosurfaceEdgesOffset",
                                                vtkm::cont::Field::Association::WholeDataSet,
                                                b->IsosurfaceEdgesOffset);
    outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeOffsetField);

    vtkm::cont::Field IsosurfaceEdgeOrderField("IsosurfaceEdgesOrders",
                                               vtkm::cont::Field::Association::WholeDataSet,
                                               b->IsosurfaceEdgesOrders);
    outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeOrderField);
    vtkm::cont::Field IsosurfaceIsoValueField(
      "IsosurfaceIsoValue", vtkm::cont::Field::Association::WholeDataSet, b->IsosurfaceIsoValue);
    outputDataSets[b->LocalBlockNo].AddField(IsosurfaceIsoValueField);
  });

  timingsStream << "    " << std::setw(38) << std::left << "Creating Contour Extraction Output Data"
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
