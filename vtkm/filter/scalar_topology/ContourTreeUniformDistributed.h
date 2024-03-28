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


#ifndef vtk_m_filter_scalar_topology_ContourTreeUniformDistributed_h
#define vtk_m_filter_scalar_topology_ContourTreeUniformDistributed_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/BoundaryTree.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/InteriorForest.h>

#include <memory>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/scalar_topology/vtkm_filter_scalar_topology_export.h>

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{
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
class VTKM_FILTER_SCALAR_TOPOLOGY_EXPORT ContourTreeUniformDistributed : public vtkm::filter::Filter
{
public:
  VTKM_CONT bool CanThread() const override
  {
    // tons of shared mutable states
    return false;
  }

  ContourTreeUniformDistributed(vtkm::cont::LogLevel timingsLogLevel = vtkm::cont::LogLevel::Perf,
                                vtkm::cont::LogLevel treeLogLevel = vtkm::cont::LogLevel::Info);

  VTKM_CONT void SetUseBoundaryExtremaOnly(bool useBoundaryExtremaOnly)
  {
    this->UseBoundaryExtremaOnly = useBoundaryExtremaOnly;
  }

  VTKM_CONT bool GetUseBoundaryExtremaOnly() { return this->UseBoundaryExtremaOnly; }

  VTKM_CONT void SetUseMarchingCubes(bool useMarchingCubes)
  {
    this->UseMarchingCubes = useMarchingCubes;
  }

  VTKM_CONT bool GetUseMarchingCubes() { return this->UseMarchingCubes; }

  VTKM_CONT void SetAugmentHierarchicalTree(bool augmentHierarchicalTree)
  {
    this->AugmentHierarchicalTree = augmentHierarchicalTree;
  }

  VTKM_CONT void SetBlockIndices(vtkm::Id3 blocksPerDim,
                                 const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices)
  {
    this->BlocksPerDimension = blocksPerDim;
    vtkm::cont::ArrayCopy(localBlockIndices, this->LocalBlockIndices);
  }

  VTKM_CONT bool GetAugmentHierarchicalTree() { return this->AugmentHierarchicalTree; }

  VTKM_CONT void SetSaveDotFiles(bool saveDotFiles) { this->SaveDotFiles = saveDotFiles; }

  VTKM_CONT bool GetSaveDotFiles() { return this->SaveDotFiles; }

  template <typename T, typename StorageType>
  VTKM_CONT void ComputeLocalTree(const vtkm::Id blockIndex,
                                  const vtkm::cont::DataSet& input,
                                  const vtkm::cont::ArrayHandle<T, StorageType>& fieldArray);

  /// Implement per block contour tree computation after the MeshType has been discovered
  template <typename T, typename StorageType, typename MeshType, typename MeshBoundaryExecType>
  VTKM_CONT void ComputeLocalTreeImpl(const vtkm::Id blockIndex,
                                      const vtkm::cont::DataSet& input,
                                      const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                      MeshType& mesh,
                                      MeshBoundaryExecType& meshBoundaryExecObject);

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;

  ///@{
  /// when operating on vtkm::cont::MultiBlock we want to
  /// do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VTKM_CONT void PreExecute(const vtkm::cont::PartitionedDataSet& input);

  VTKM_CONT void PostExecute(const vtkm::cont::PartitionedDataSet& input,
                             vtkm::cont::PartitionedDataSet& output);


  template <typename FieldType>
  VTKM_CONT void ComputeVolumeMetric(
    vtkmdiy::Master& inputContourTreeMaster,
    vtkmdiy::DynamicAssigner& assigner,
    vtkmdiy::RegularSwapPartners& partners,
    const FieldType&, // dummy parameter to get the type
    std::stringstream& timingsStream,
    std::vector<vtkm::cont::DataSet>& hierarchicalTreeOutputDataSet);

  ///
  /// Internal helper function that implements the actual functionality of PostExecute
  ///
  /// In the case we operate on vtkm::cont::MultiBlock we need to merge the trees
  /// computed on the block to compute the final contour tree.
  template <typename T>
  VTKM_CONT void DoPostExecute(const vtkm::cont::PartitionedDataSet& input,
                               vtkm::cont::PartitionedDataSet& output);
  ///@}

  /// Use only boundary critical points in the parallel merge to reduce communication.
  /// Disabling this should only be needed for performance testing.
  bool UseBoundaryExtremaOnly;

  /// Use marching cubes connectivity for computing the contour tree
  bool UseMarchingCubes;

  /// Augment hierarchical tree
  bool AugmentHierarchicalTree;

  /// Save dot files for all tree computations
  bool SaveDotFiles;

  /// Log level to be used for outputting timing information. Default is vtkm::cont::LogLevel::Perf
  vtkm::cont::LogLevel TimingsLogLevel = vtkm::cont::LogLevel::Perf;

  /// Log level to be used for outputting metadata about the trees. Default is vtkm::cont::LogLevel::Info
  vtkm::cont::LogLevel TreeLogLevel = vtkm::cont::LogLevel::Info;

  /// Information about block decomposition TODO/FIXME: Remove need for this information
  // ... Number of blocks along each dimension
  vtkm::Id3 BlocksPerDimension;
  // ... Index of the local blocks in x,y,z, i.e., in i,j,k mesh coordinates
  vtkm::cont::ArrayHandle<vtkm::Id3> LocalBlockIndices;

  /// Intermediate results (one per local data block)...
  /// ... local mesh information needed at end of fan out
  std::vector<vtkm::worklet::contourtree_augmented::DataSetMesh> LocalMeshes;
  /// ... local contour trees etc. computed during fan in and used during fan out
  std::vector<vtkm::worklet::contourtree_augmented::ContourTree> LocalContourTrees;
  std::vector<vtkm::worklet::contourtree_distributed::BoundaryTree> LocalBoundaryTrees;
  std::vector<vtkm::worklet::contourtree_distributed::InteriorForest> LocalInteriorForests;

  /// The hierarchical trees computed by the filter (array with one entry per block)
  // TODO/FIXME: We need to find a way to store the final hieararchical trees somewhere.
  // Currently we cannot do this here as it is a template on FieldType
  //
  //std::vector<vtkm::worklet::contourtree_distributed::HierarchicalContourTree> HierarchicalContourTrees;
  /// Number of iterations used to compute the contour tree
  vtkm::Id NumIterations;
};
} // namespace scalar_topology
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_scalar_topology_ContourTreeUniformDistributed_h
