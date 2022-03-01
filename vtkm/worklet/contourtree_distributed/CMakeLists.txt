##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  BoundaryTree.h
  BoundaryTreeMaker.h
  BranchDecompositionBlock.h
  CombineHyperSweepBlockFunctor.h
  ComputeDistributedBranchDecompositionFunctor.h
  ComputeDistributedContourTreeFunctor.h
  ContourTreeBlockData.h
  DistributedContourTreeBlockData.h
  HierarchicalAugmenter.h
  HierarchicalAugmenterFunctor.h
  HierarchicalVolumetricBranchDecomposer.h
  HierarchicalContourTree.h
  HierarchicalHyperSweeper.h
  HyperSweepBlock.h
  InteriorForest.h
  MergeBlockFunctor.h
  MultiBlockContourTreeHelper.h
  PrintGraph.h
  SpatialDecomposition.h
  TreeCompiler.h
  TreeGrafter.h
)

vtkm_declare_headers(${headers})

add_subdirectory(boundary_tree_maker)
add_subdirectory(tree_grafter)
add_subdirectory(hierarchical_contour_tree)
add_subdirectory(hierarchical_hyper_sweeper)
add_subdirectory(hierarchical_augmenter)
add_subdirectory(hierarchical_volumetric_branch_decomposer)
