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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/internal/ComputeBlockIndices.h>
#include <viskores/filter/scalar_topology/worklet/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/Configure.h>
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/ContourTreeBlockData.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/MergeBlockFunctor.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/MultiBlockContourTreeHelper.h>

#include <memory>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

//-----------------------------------------------------------------------------
ContourTreeAugmented::ContourTreeAugmented()
  : MultiBlockTreeHelper(nullptr)
{
  this->SetOutputFieldName("Superparents");
}

// Deprecated constructor kept for backward compatibility
ContourTreeAugmented::ContourTreeAugmented(bool useMarchingCubes,
                                           unsigned int computeRegularStructure)
  : UseMarchingCubes(useMarchingCubes)
  , AugmentTree(computeRegularStructure)
  , MultiBlockTreeHelper(nullptr)
{
  this->SetOutputFieldName("Superparents");
}

ContourTreeAugmented::ContourTreeAugmented(ContourTreeAugmented&&) = default;

ContourTreeAugmented::~ContourTreeAugmented() {}

std::string ContourTreeAugmented::GetContourTreeStatisticsString() const
{
  return this->ContourTreeData.PrintArraySizes() +
    this->ContourTreeData.PrintHyperStructureStatistics(false);
}

void ContourTreeAugmented::SetBlockIndices(
  viskores::Id3 blocksPerDim,
  const viskores::cont::ArrayHandle<viskores::Id3>& localBlockIndices)
{
  if (this->MultiBlockTreeHelper)
  {
    this->MultiBlockTreeHelper.reset();
  }
  this->MultiBlockTreeHelper =
    std::make_unique<viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper>(
      blocksPerDim, localBlockIndices);
}

const viskores::worklet::contourtree_augmented::ContourTree& ContourTreeAugmented::GetContourTree()
  const
{
  return this->ContourTreeData;
}

const viskores::worklet::contourtree_augmented::IdArrayType& ContourTreeAugmented::GetSortOrder()
  const
{
  return this->MeshSortOrder;
}

viskores::Id ContourTreeAugmented::GetNumIterations() const
{
  return this->NumIterations;
}

//-----------------------------------------------------------------------------
void ContourTreeAugmented::PopulateOutputDataSet(
  const viskores::worklet::contourtree_augmented::ContourTree& ct,
  const viskores::worklet::contourtree_augmented::IdArrayType& sortOrder,
  viskores::Id numIterations,
  viskores::cont::DataSet& output)
{
  using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;
  namespace cta = viskores::worklet::contourtree_augmented;

  // Supernodes in mesh vertex space: sortOrder[ct.Supernodes[i]]
  IdArrayType supernodesInMesh;
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandlePermutation(ct.Supernodes, sortOrder), supernodesInMesh);
  output.AddField(viskores::cont::Field(
    "Supernodes", viskores::cont::Field::Association::WholeDataSet, supernodesInMesh));

  // Superarcs: destination supernode index per supernode (high bit encodes arc direction)
  output.AddField(viskores::cont::Field(
    "Superarcs", viskores::cont::Field::Association::WholeDataSet, ct.Superarcs));

  // Superparents as a mesh-order point field: scatter ct.Superparents[sortedIdx] → meshVertex
  if (this->AugmentTree >= 1)
  {
    IdArrayType superparentsInMesh;
    superparentsInMesh.Allocate(ct.Superparents.GetNumberOfValues());
    {
      auto inPortal = ct.Superparents.ReadPortal();
      auto mapPortal = sortOrder.ReadPortal();
      auto outPortal = superparentsInMesh.WritePortal();
      for (viskores::Id i = 0; i < inPortal.GetNumberOfValues(); ++i)
        outPortal.Set(mapPortal.Get(i), inPortal.Get(i));
    }
    output.AddField(viskores::cont::Field(
      this->GetOutputFieldName(), viskores::cont::Field::Association::Points, superparentsInMesh));
  }

  if (this->ComputeBranchDecompositionFlag && this->AugmentTree >= 1)
  {
    IdArrayType superarcIntrinsicWeight, superarcDependentWeight;
    IdArrayType supernodeTransferWeight, hyperarcDependentWeight;
    cta::ProcessContourTree::ComputeVolumeWeightsSerial(ct,
                                                        numIterations,
                                                        superarcIntrinsicWeight,
                                                        superarcDependentWeight,
                                                        supernodeTransferWeight,
                                                        hyperarcDependentWeight);

    IdArrayType whichBranch, branchMinimum, branchMaximum, branchSaddle, branchParent;
    cta::ProcessContourTree::ComputeVolumeBranchDecompositionSerial(ct,
                                                                    superarcDependentWeight,
                                                                    superarcIntrinsicWeight,
                                                                    whichBranch,
                                                                    branchMinimum,
                                                                    branchMaximum,
                                                                    branchSaddle,
                                                                    branchParent);

    output.AddField(viskores::cont::Field(
      "WhichBranch", viskores::cont::Field::Association::WholeDataSet, whichBranch));
    output.AddField(viskores::cont::Field(
      "BranchMinimum", viskores::cont::Field::Association::WholeDataSet, branchMinimum));
    output.AddField(viskores::cont::Field(
      "BranchMaximum", viskores::cont::Field::Association::WholeDataSet, branchMaximum));
    output.AddField(viskores::cont::Field(
      "BranchSaddle", viskores::cont::Field::Association::WholeDataSet, branchSaddle));
    output.AddField(viskores::cont::Field(
      "BranchParent", viskores::cont::Field::Association::WholeDataSet, branchParent));
  }
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet ContourTreeAugmented::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::cont::Timer timer;
  timer.Start();

  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  viskores::Id3 meshSize;
  const auto& cells = input.GetCellSet();
  cells.CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetPointDimensions(), meshSize);

  std::size_t blockIndex = 0;

  unsigned int compRegularStruct = this->AugmentTree;
  if (compRegularStruct == 0)
  {
    if (this->MultiBlockTreeHelper)
    {
      if (this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks() > 1)
      {
        compRegularStruct = 2;
      }
    }
  }

  viskores::cont::DataSet result;

  auto resolveType = [&](const auto& concrete)
  {
    viskores::worklet::ContourTreeAugmented worklet;
    worklet.Run(concrete,
                MultiBlockTreeHelper ? MultiBlockTreeHelper->LocalContourTrees[blockIndex]
                                     : this->ContourTreeData,
                MultiBlockTreeHelper ? MultiBlockTreeHelper->LocalSortOrders[blockIndex]
                                     : this->MeshSortOrder,
                this->NumIterations,
                meshSize,
                this->UseMarchingCubes,
                compRegularStruct);

    // Copy input DataSet (geometry + all fields) then add contour tree fields.
    // Multi-block cases are handled in PostExecute.
    result = input;
    if (!this->MultiBlockTreeHelper)
    {
      this->PopulateOutputDataSet(
        this->ContourTreeData, this->MeshSortOrder, this->NumIterations, result);
    }
  };
  this->CastAndCallScalarField(field, resolveType);

  VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                 std::endl
                   << "    " << std::setw(38) << std::left << "Contour Tree Filter DoExecute"
                   << ": " << timer.GetElapsedTime() << " seconds");

  return result;
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::PartitionedDataSet ContourTreeAugmented::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  this->PreExecute(input);
  auto result = this->Filter::DoExecutePartitions(input);
  this->PostExecute(input, result);
  return result;
}

//-----------------------------------------------------------------------------
VISKORES_CONT void ContourTreeAugmented::PreExecute(const viskores::cont::PartitionedDataSet& input)
{
  if (this->MultiBlockTreeHelper)
  {
    if (input.GetGlobalNumberOfPartitions() !=
        this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks())
    {
      throw viskores::cont::ErrorFilterExecution(
        "Global number of block in MultiBlock dataset does not match the SpatialDecomposition");
    }
    if (this->MultiBlockTreeHelper->GetLocalNumberOfBlocks() != input.GetNumberOfPartitions())
    {
      throw viskores::cont::ErrorFilterExecution(
        "Global number of block in MultiBlock dataset does not match the SpatialDecomposition");
    }
  }
  else
  {
    this->MultiBlockTreeHelper =
      std::make_unique<viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper>(
        input);
  }
}

//-----------------------------------------------------------------------------
template <typename T>
VISKORES_CONT void ContourTreeAugmented::DoPostExecute(
  const viskores::cont::PartitionedDataSet& input,
  viskores::cont::PartitionedDataSet& output)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id size = comm.size();
  viskores::Id rank = comm.rank();

  std::vector<viskores::worklet::contourtree_augmented::ContourTreeMesh<T>*> localContourTreeMeshes;
  localContourTreeMeshes.resize(static_cast<std::size_t>(input.GetNumberOfPartitions()));
  std::vector<viskores::worklet::contourtree_distributed::ContourTreeBlockData<T>*> localDataBlocks;
  localDataBlocks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  std::vector<viskoresdiy::Link*> localLinks;
  localLinks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  unsigned int compRegularStruct = (this->AugmentTree > 0) ? this->AugmentTree : 2;

  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    localLinks[bi] = new viskoresdiy::Link;
    auto currBlock = input.GetPartition(static_cast<viskores::Id>(bi));
    auto currField =
      currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());

    viskores::Id3 pointDimensions, globalPointDimensions, globalPointIndexStart;
    currBlock.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
      viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
      pointDimensions,
      globalPointDimensions,
      globalPointIndexStart);

    viskores::cont::ArrayHandle<T> fieldData;
    viskores::cont::ArrayCopy(currField.GetData(), fieldData);
    auto currContourTreeMesh =
      viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper::
        ComputeLocalContourTreeMesh<T>(globalPointIndexStart,
                                       pointDimensions,
                                       globalPointDimensions,
                                       fieldData,
                                       MultiBlockTreeHelper->LocalContourTrees[bi],
                                       MultiBlockTreeHelper->LocalSortOrders[bi],
                                       compRegularStruct);
    localContourTreeMeshes[bi] = currContourTreeMesh;
    localDataBlocks[bi] = new viskores::worklet::contourtree_distributed::ContourTreeBlockData<T>();
    localDataBlocks[bi]->NumVertices = currContourTreeMesh->NumVertices;
    localDataBlocks[bi]->SortedValue = currContourTreeMesh->SortedValues;
    localDataBlocks[bi]->GlobalMeshIndex = currContourTreeMesh->GlobalMeshIndex;
    localDataBlocks[bi]->NeighborConnectivity = currContourTreeMesh->NeighborConnectivity;
    localDataBlocks[bi]->NeighborOffsets = currContourTreeMesh->NeighborOffsets;
    localDataBlocks[bi]->MaxNeighbors = currContourTreeMesh->MaxNeighbors;
    localDataBlocks[bi]->BlockOrigin = globalPointIndexStart;
    localDataBlocks[bi]->BlockSize = pointDimensions;
    localDataBlocks[bi]->GlobalSize = globalPointDimensions;
    localDataBlocks[bi]->ComputeRegularStructure = compRegularStruct;
  }

  viskoresdiy::Master master(comm, 1, -1);

  using RegularDecomposer = viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds>;

  RegularDecomposer::DivisionsVector diyDivisions;
  std::vector<int> viskoresdiyLocalBlockGids;
  viskoresdiy::DiscreteBounds diyBounds(0);
  if (this->MultiBlockTreeHelper->BlocksPerDimension[0] == -1)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "BlocksPerDimension not set. Computing block indices "
                   "from information in CellSetStructured.");
    diyBounds = viskores::filter::scalar_topology::internal::ComputeBlockIndices(
      input, diyDivisions, viskoresdiyLocalBlockGids);
  }
  else
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "BlocksPerDimension set. Using information provided by caller.");
    diyBounds = viskores::filter::scalar_topology::internal::ComputeBlockIndices(
      input,
      this->MultiBlockTreeHelper->BlocksPerDimension,
      this->MultiBlockTreeHelper->LocalBlockIndices,
      diyDivisions,
      viskoresdiyLocalBlockGids);
  }
  int numDims = diyBounds.min.dimension();
  int globalNumberOfBlocks =
    std::accumulate(diyDivisions.cbegin(), diyDivisions.cend(), 1, std::multiplies<int>{});

  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    master.add(
      static_cast<int>(viskoresdiyLocalBlockGids[bi]), localDataBlocks[bi], localLinks[bi]);
  }

  RegularDecomposer::BoolVector shareFace(3, true);
  RegularDecomposer::BoolVector wrap(3, false);
  RegularDecomposer::CoordinateVector ghosts(3, 1);
  RegularDecomposer decomposer(static_cast<int>(numDims),
                               diyBounds,
                               globalNumberOfBlocks,
                               shareFace,
                               wrap,
                               ghosts,
                               diyDivisions);

  viskoresdiy::DynamicAssigner assigner(comm, static_cast<int>(size), globalNumberOfBlocks);
  for (viskores::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
    assigner.set_rank(static_cast<int>(rank),
                      static_cast<int>(viskoresdiyLocalBlockGids[static_cast<size_t>(bi)]));
  }

  viskoresdiy::fix_links(master, assigner);

  viskoresdiy::RegularMergePartners partners(decomposer, 2, true);
  viskoresdiy::reduce(
    master, assigner, partners, &viskores::worklet::contourtree_distributed::MergeBlockFunctor<T>);

  comm.barrier();

  if (rank == 0)
  {
    viskores::Id3 dummy1, globalPointDimensions, dummy2;
    viskores::cont::DataSet firstDS = input.GetPartition(0);
    firstDS.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
      viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
      dummy1,
      globalPointDimensions,
      dummy2);
    viskores::Id currNumIterations;
    viskores::worklet::contourtree_augmented::IdArrayType currSortOrder;
    viskores::worklet::ContourTreeAugmented worklet;
    viskores::worklet::contourtree_augmented::ContourTreeMesh<T> contourTreeMeshOut;
    contourTreeMeshOut.NumVertices = localDataBlocks[0]->NumVertices;
    contourTreeMeshOut.SortOrder = viskores::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
    contourTreeMeshOut.SortIndices =
      viskores::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
    contourTreeMeshOut.SortedValues = localDataBlocks[0]->SortedValue;
    contourTreeMeshOut.GlobalMeshIndex = localDataBlocks[0]->GlobalMeshIndex;
    contourTreeMeshOut.NeighborConnectivity = localDataBlocks[0]->NeighborConnectivity;
    contourTreeMeshOut.NeighborOffsets = localDataBlocks[0]->NeighborOffsets;
    contourTreeMeshOut.MaxNeighbors = localDataBlocks[0]->MaxNeighbors;
    viskores::Id3 minIdx(0, 0, 0);
    viskores::Id3 maxIdx = globalPointDimensions;
    maxIdx[0] = maxIdx[0] - 1;
    maxIdx[1] = maxIdx[1] - 1;
    maxIdx[2] = maxIdx[2] > 0 ? (maxIdx[2] - 1) : 0;
    auto meshBoundaryExecObj =
      contourTreeMeshOut.GetMeshBoundaryExecutionObject(globalPointDimensions, minIdx, maxIdx);
    worklet.Run(contourTreeMeshOut.SortedValues,
                contourTreeMeshOut,
                this->ContourTreeData,
                this->MeshSortOrder,
                currNumIterations,
                this->AugmentTree,
                meshBoundaryExecObj);

    this->MeshSortOrder = contourTreeMeshOut.GlobalMeshIndex;
    this->NumIterations = currNumIterations;

    viskores::cont::DataSet temp;
    this->PopulateOutputDataSet(
      this->ContourTreeData, this->MeshSortOrder, this->NumIterations, temp);

    // Add the scalar field in mesh-vertex order so downstream callers (e.g.,
    // SelectTopVolumeBranches) can access data values without the sort order.
    // SortedValues[i] is the value at global mesh vertex GlobalMeshIndex[i].
    viskores::cont::ArrayHandle<T> meshData;
    meshData.Allocate(contourTreeMeshOut.NumVertices);
    {
      auto sortedPortal = contourTreeMeshOut.SortedValues.ReadPortal();
      auto idxPortal = contourTreeMeshOut.GlobalMeshIndex.ReadPortal();
      auto outPortal = meshData.WritePortal();
      for (viskores::Id i = 0; i < contourTreeMeshOut.NumVertices; i++)
        outPortal.Set(idxPortal.Get(i), sortedPortal.Get(i));
    }
    temp.AddField(viskores::cont::Field(
      this->GetActiveFieldName(), viskores::cont::Field::Association::Points, meshData));

    output = viskores::cont::PartitionedDataSet(temp);
  }
  else
  {
    this->ContourTreeData = MultiBlockTreeHelper->LocalContourTrees[0];
    this->MeshSortOrder = MultiBlockTreeHelper->LocalSortOrders[0];

    for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
    {
      delete localContourTreeMeshes[bi];
      delete localDataBlocks[bi];
    }
  }
  localContourTreeMeshes.clear();
  localDataBlocks.clear();
  localLinks.clear();
}

//-----------------------------------------------------------------------------
VISKORES_CONT void ContourTreeAugmented::PostExecute(
  const viskores::cont::PartitionedDataSet& input,
  viskores::cont::PartitionedDataSet& result)
{
  if (this->MultiBlockTreeHelper)
  {
    viskores::cont::Timer timer;
    timer.Start();

    auto field =
      input.GetPartition(0).GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());

    auto PostExecuteCaller = [&](const auto& concrete)
    {
      using T = typename std::decay_t<decltype(concrete)>::ValueType;
      this->DoPostExecute<T>(input, result);
    };
    this->CastAndCallScalarField(field, PostExecuteCaller);

    this->MultiBlockTreeHelper.reset();
    VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                   std::endl
                     << "    " << std::setw(38) << std::left << "Contour Tree Filter PostExecute"
                     << ": " << timer.GetElapsedTime() << " seconds");
  }
}

} // namespace scalar_topology
} // namespace filter
} // namespace viskores
