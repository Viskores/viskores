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

#ifndef viskores_filter_scalar_topology_ContourTreeUniformAugmented_h
#define viskores_filter_scalar_topology_ContourTreeUniformAugmented_h

#include <viskores/Deprecated.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/scalar_topology/viskores_filter_scalar_topology_export.h>

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>

#include <memory>
#include <ostream>
#include <string>
#include <vector>

// Forward declaration
namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
class MultiBlockContourTreeHelper;
}
}
}

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

/// \brief Summary statistics of a contour tree computed by ContourTreeAugmented.
///
/// Returned by ContourTreeAugmented::GetContourTreeStatistics() after Execute().
/// The array sizes describe the regular, super-, and hyper-structure of the
/// tree; Iterations summarizes the hyperarc path lengths for each iteration of
/// the algorithm. Use ToString() (or stream the object) for a formatted,
/// loggable rendering.
struct VISKORES_FILTER_SCALAR_TOPOLOGY_EXPORT ContourTreeStatistics
{
  /// Hyperstructure summary for one iteration of the contour tree algorithm.
  struct IterationStatistics
  {
    viskores::Id NumHypernodes = 0;
    viskores::Id NumSupernodes = 0;
    viskores::Id MinPathLength = 0;
    viskores::Float64 AveragePathLength = 0;
    viskores::Id MaxPathLength = 0;
  };

  viskores::Id NumNodes = 0;
  viskores::Id NumArcs = 0;
  viskores::Id NumSuperparents = 0;
  viskores::Id NumSuperarcs = 0;
  viskores::Id NumSupernodes = 0;
  viskores::Id NumHyperparents = 0;
  viskores::Id NumWhenTransferred = 0;
  viskores::Id NumHypernodes = 0;
  viskores::Id NumHyperarcs = 0;
  std::vector<IterationStatistics> Iterations;

  VISKORES_CONT std::string ToString() const;
};

inline std::ostream& operator<<(std::ostream& os, const ContourTreeStatistics& statistics)
{
  return os << statistics.ToString();
}

/// \brief Construct the Contour Tree for a 2D or 3D regular mesh
///
/// This filter implements the parallel peak pruning algorithm. In contrast to
/// the ContourTreeUniform filter, this filter is optimized to allow for the
/// computation of the augmented contour tree, i.e., the contour tree including
/// all regular mesh vertices. Augmentation with regular vertices is used in
/// practice to compute statistics (e.g., volume), to segment the input mesh,
/// facilitate iso-value selection, enable localization of all verticies of a
/// mesh in the tree among others.
///
/// For a single `DataSet` the filter returns its results as fields on the
/// output `DataSet`: `Supernodes` and `Superarcs` are always present; with
/// `SetAugmentTree(false)` (see below) the output contains only those two
/// fields. `Superparents` is added only when the tree is augmented, and the
/// branch-decomposition fields (`WhichBranch`, `BranchMinimum`,
/// `BranchMaximum`, `BranchSaddle`, `BranchParent`) only when
/// `SetComputeBranchDecomposition(true)` was set. The `ProcessContourTree`
/// `DataSet` helpers that require `Superparents` (`CollectRegularVerticesPerSuperarc`,
/// `SelectTopVolumeBranches`, `ComputeBranchDecomposition`) throw a
/// missing-field error if the tree was not augmented.
///
/// All output arrays are indexed by (or contain) mesh vertex IDs; no separate
/// sort order is needed to interpret them. For downstream algorithms that want
/// to work in the sorted space the tree was computed in, `SetIncludeSortOrder(true)`
/// additionally exports the `SortOrder` field (the sorted-to-mesh-index
/// permutation), from which sorted-space views of the outputs can be
/// reconstructed without re-sorting the data.
///
/// The multi-block (`PartitionedDataSet`/MPI) path of this filter is
/// deprecated. It merges per-block trees on rank 0 and exposes the result only
/// through the deprecated `GetContourTree()` / `GetSortOrder()` /
/// `GetNumIterations()` getters. For multi-block data use
/// `ContourTreeUniformDistributed` together with
/// `DistributedBranchDecompositionFilter`,
/// `SelectTopVolumeBranchesDistributedFilter`, and
/// `ExtractTopVolumeContoursFilter` instead.
class VISKORES_FILTER_SCALAR_TOPOLOGY_EXPORT ContourTreeAugmented : public viskores::filter::Filter
{
public:
  VISKORES_CONT bool CanThread() const override { return false; }

  VISKORES_CONT ContourTreeAugmented();

  // Required for incomplete type MultiBlockContourTreeHelper
  ContourTreeAugmented(ContourTreeAugmented&& src);
  ~ContourTreeAugmented();

  /// @deprecated Use the default constructor and SetUseMarchingCubes / SetAugmentTree.
  VISKORES_DEPRECATED(1.3, "Use default constructor + SetUseMarchingCubes / SetAugmentTree.")
  VISKORES_CONT explicit ContourTreeAugmented(bool useMarchingCubes,
                                              unsigned int computeRegularStructure = 1);

  VISKORES_CONT void SetUseMarchingCubes(bool v) { this->UseMarchingCubes = v; }
  VISKORES_CONT bool GetUseMarchingCubes() const { return this->UseMarchingCubes; }

  /// Enable/disable augmentation of the contour tree with regular mesh vertices.
  /// When enabled (the default), the output additionally carries the `Superparents`
  /// point field mapping every mesh vertex to its superarc.
  VISKORES_CONT void SetAugmentTree(bool v) { this->AugmentationLevel = v ? 1u : 0u; }
  VISKORES_CONT bool GetAugmentTree() const { return this->AugmentationLevel == 1u; }

  /// Enable/disable volume-based branch decomposition output.
  /// When enabled, writes "WhichBranch", "BranchMinimum", "BranchMaximum", "BranchSaddle",
  /// and "BranchParent" arrays to the output DataSet. Branch decomposition requires an
  /// augmented contour tree; the constraint is enforced at Execute() time.
  VISKORES_CONT void SetComputeBranchDecomposition(bool v)
  {
    this->ComputeBranchDecompositionFlag = v;
  }
  VISKORES_CONT bool GetComputeBranchDecomposition() const
  {
    return this->ComputeBranchDecompositionFlag;
  }

  /// Enable/disable exporting the mesh sort order as a `SortOrder` whole-dataset field.
  /// `SortOrder[i]` is the mesh vertex ID of the i-th vertex in sorted order, i.e., the
  /// (value, mesh index) lexicographic order the contour tree was computed with. Off by
  /// default because it adds an array with one entry per mesh vertex that most consumers
  /// do not need. Downstream algorithms that work in sorted space can invert this
  /// permutation with a single scatter instead of re-sorting the data.
  VISKORES_CONT void SetIncludeSortOrder(bool v) { this->IncludeSortOrderFlag = v; }
  VISKORES_CONT bool GetIncludeSortOrder() const { return this->IncludeSortOrderFlag; }

  /// Returns summary statistics (array sizes and per-iteration hyperstructure) of the
  /// contour tree computed by the last Execute() call.
  VISKORES_CONT ContourTreeStatistics GetContourTreeStatistics() const;

  /// @deprecated Define the spatial decomposition for multi-block datasets. The
  /// multi-block path of ContourTreeAugmented is deprecated; use ContourTreeUniformDistributed.
  VISKORES_DEPRECATED(1.3,
                      "The multi-block/MPI path of ContourTreeAugmented is deprecated. "
                      "Use ContourTreeUniformDistributed.")
  VISKORES_CONT void SetBlockIndices(
    viskores::Id3 blocksPerDim,
    const viskores::cont::ArrayHandle<viskores::Id3>& localBlockIndices);

  /// @deprecated Needed only for the deprecated multi-block path. For single-DataSet
  /// execution, read the output DataSet fields.
  VISKORES_DEPRECATED(1.3,
                      "Needed only for the deprecated multi-block path. For single-DataSet "
                      "execution, read the output DataSet fields.")
  const viskores::worklet::contourtree_augmented::ContourTree& GetContourTree() const;

  /// @deprecated Needed only for the deprecated multi-block path. For single-DataSet
  /// execution, read the output DataSet fields.
  VISKORES_DEPRECATED(1.3,
                      "Needed only for the deprecated multi-block path. For single-DataSet "
                      "execution, read the output DataSet fields.")
  const viskores::worklet::contourtree_augmented::IdArrayType& GetSortOrder() const;

  /// @deprecated Needed only for the deprecated multi-block path. For single-DataSet
  /// execution, read the output DataSet fields.
  VISKORES_DEPRECATED(1.3,
                      "Needed only for the deprecated multi-block path. For single-DataSet "
                      "execution, read the output DataSet fields.")
  viskores::Id GetNumIterations() const;

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& inData) override;

  VISKORES_CONT void PreExecute(const viskores::cont::PartitionedDataSet& input);
  VISKORES_CONT void PostExecute(const viskores::cont::PartitionedDataSet& input,
                                 viskores::cont::PartitionedDataSet& output);

  template <typename T>
  VISKORES_CONT void DoPostExecute(const viskores::cont::PartitionedDataSet& input,
                                   viskores::cont::PartitionedDataSet& output);

  /// Writes contour tree fields (and optionally branch decomposition fields) to output.
  VISKORES_CONT void PopulateOutputDataSet(
    const viskores::worklet::contourtree_augmented::ContourTree& ct,
    const viskores::worklet::contourtree_augmented::IdArrayType& sortOrder,
    viskores::Id numIterations,
    unsigned int augmentationLevel,
    viskores::cont::DataSet& output);

  bool UseMarchingCubes = false;
  // Augmentation level: 0 = none, 1 = full, 2 = boundary (2 reachable only via the
  // deprecated constructor, for the deprecated multi-block path).
  unsigned int AugmentationLevel = 1;
  bool ComputeBranchDecompositionFlag = false;
  bool IncludeSortOrderFlag = false;

  // Kept to support deprecated getters during the transition period.
  viskores::worklet::contourtree_augmented::ContourTree ContourTreeData;
  viskores::Id NumIterations = 0;
  viskores::worklet::contourtree_augmented::IdArrayType MeshSortOrder;

  std::unique_ptr<viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper>
    MultiBlockTreeHelper;
};
} // namespace scalar_topology
} // namespace filter
} // namespace viskores

#endif // viskores_filter_scalar_topology_ContourTreeUniformAugmented_h
