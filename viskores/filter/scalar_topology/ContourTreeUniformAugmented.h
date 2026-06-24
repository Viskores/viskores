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

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Logging.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/scalar_topology/viskores_filter_scalar_topology_export.h>

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>

#include <memory>

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

/// Augmentation level for ContourTreeAugmented.
enum class AugmentationType : unsigned int
{
  NoAugmentation = 0,      ///< No augmentation; only supernodes and superarcs are computed.
  FullAugmentation = 1,    ///< Full augmentation; all regular mesh vertices are added.
  BoundaryAugmentation = 2 ///< Boundary-only augmentation (multi-block efficiency optimization).
};

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
/// In addition to single-block computation, the filter also supports multi-block
/// regular grids. The blocks are processed in parallel using DIY and then the
/// tree are merged progressively using a binary-reduction scheme to compute the
/// final contour tree. I.e., in the multi-block context, the final tree is
/// constructed on rank 0.
class VISKORES_FILTER_SCALAR_TOPOLOGY_EXPORT ContourTreeAugmented : public viskores::filter::Filter
{
public:
  VISKORES_CONT bool CanThread() const override { return false; }

  VISKORES_CONT ContourTreeAugmented();

  // Required for incomplete type MultiBlockContourTreeHelper
  ContourTreeAugmented(ContourTreeAugmented&& src);
  ~ContourTreeAugmented();

  /// @deprecated Use the default constructor and SetUseMarchingCubes / SetComputeRegularStructure.
  [[deprecated(
    "Use default constructor + SetUseMarchingCubes / "
    "SetComputeRegularStructure")]] VISKORES_CONT explicit ContourTreeAugmented(bool
                                                                                  useMarchingCubes,
                                                                                unsigned int
                                                                                  computeRegularStructure =
                                                                                    1);

  VISKORES_CONT void SetUseMarchingCubes(bool v) { this->UseMarchingCubes = v; }
  VISKORES_CONT bool GetUseMarchingCubes() const { return this->UseMarchingCubes; }

  VISKORES_CONT void SetAugmentTree(AugmentationType v)
  {
    this->AugmentTree = static_cast<unsigned int>(v);
  }
  VISKORES_CONT AugmentationType GetAugmentTree() const
  {
    return static_cast<AugmentationType>(this->AugmentTree);
  }

  /// @deprecated Use SetAugmentTree(AugmentationType).
  [[deprecated("Use SetAugmentTree(AugmentationType)")]] VISKORES_CONT void
  SetComputeRegularStructure(unsigned int v)
  {
    this->AugmentTree = v;
  }
  /// @deprecated Use GetAugmentTree().
  [[deprecated("Use GetAugmentTree()")]] VISKORES_CONT unsigned int GetComputeRegularStructure()
    const
  {
    return this->AugmentTree;
  }

  /// Enable/disable volume-based branch decomposition output.
  /// When enabled, writes "WhichBranch", "BranchMinimum", "BranchMaximum", "BranchSaddle",
  /// and "BranchParent" arrays to the output DataSet.
  /// Requires FullAugmentation. If branch decomposition is enabled but AugmentTree is not
  /// FullAugmentation, AugmentTree is automatically set to FullAugmentation and a warning
  /// is logged.
  VISKORES_CONT void SetComputeBranchDecomposition(bool v)
  {
    this->ComputeBranchDecompositionFlag = v;
    if (v && this->AugmentTree != static_cast<unsigned int>(AugmentationType::FullAugmentation))
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                     "SetComputeBranchDecomposition(true) requires FullAugmentation; "
                     "AugmentTree has been set to FullAugmentation.");
      this->AugmentTree = static_cast<unsigned int>(AugmentationType::FullAugmentation);
    }
  }
  VISKORES_CONT bool GetComputeBranchDecomposition() const
  {
    return this->ComputeBranchDecompositionFlag;
  }

  /// Returns a formatted string with internal contour tree array sizes and hyperstructure
  /// statistics, suitable for logging. Only valid after Execute() has been called.
  VISKORES_CONT std::string GetContourTreeStatisticsString() const;

  /// Define the spatial decomposition for multi-block datasets.
  VISKORES_CONT void SetBlockIndices(
    viskores::Id3 blocksPerDim,
    const viskores::cont::ArrayHandle<viskores::Id3>& localBlockIndices);

  /// @deprecated Read "Supernodes" / "Superarcs" fields from the Execute() output DataSet.
  [[deprecated(
    "Read 'Supernodes'/'Superarcs' fields from the Execute() output DataSet")]] const viskores::
    worklet::contourtree_augmented::ContourTree&
    GetContourTree() const;

  /// @deprecated Sort order is now an internal detail; no longer needed by callers.
  [[deprecated("Sort order is internal; no longer needed by callers")]] const viskores::worklet::
    contourtree_augmented::IdArrayType&
    GetSortOrder() const;

  /// @deprecated NumIterations is an internal detail.
  [[deprecated("NumIterations is an internal detail")]] viskores::Id GetNumIterations() const;

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
    viskores::cont::DataSet& output);

  bool UseMarchingCubes = false;
  unsigned int AugmentTree = 1; // backing store for AugmentationType (default: FullAugmentation)
  bool ComputeBranchDecompositionFlag = false;

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
