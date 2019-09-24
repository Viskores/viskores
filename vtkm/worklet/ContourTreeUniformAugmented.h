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

#ifndef vtk_m_worklet_ContourTreeUniformAugmented_h
#define vtk_m_worklet_ContourTreeUniformAugmented_h


#include <utility>
#include <vector>

// VTKM includes
#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

// Contour tree worklet includes
#include <vtkm/worklet/contourtree_augmented/ActiveGraph.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/ContourTreeMaker.h>
#include <vtkm/worklet/contourtree_augmented/MergeTree.h>
#include <vtkm/worklet/contourtree_augmented/MeshExtrema.h>
#include <vtkm/worklet/contourtree_augmented/Mesh_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/ContourTreeMesh.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/MeshBoundary.h>

namespace vtkm
{
namespace worklet
{

class ContourTreePPP2
{
public:
  /*!
  * Run the contour tree to merge an existing set of contour trees
  *
  *  meshBoundary : Is computed by calling mesh.GetMeshBoundaryExecutionObject.
  *                 It is technically only needed if computeRegularStructure==2.
  *  computeRegularStructure : 0=Off, 1=full augmentation with all vertices
  *                            2=boundary augmentation using meshBoundary
  */
  template <typename FieldType, typename StorageType>
  void Run(const vtkm::cont::ArrayHandle<FieldType, StorageType>
             fieldArray, // TODO: We really should not need this
           contourtree_augmented::ContourTreeMesh<FieldType>& mesh,
           std::vector<std::pair<std::string, vtkm::Float64>>& timings,
           contourtree_augmented::ContourTree& contourTree,
           contourtree_augmented::IdArrayType sortOrder,
           vtkm::Id& nIterations,
           unsigned int computeRegularStructure,
           const contourtree_augmented::MeshBoundaryContourTreeMeshExec& meshBoundary)
  {
    RunContourTree(
      fieldArray, // Just a place-holder to fill the required field. Used when calling SortData on the contour tree which is a no-op
      timings,
      contourTree,
      sortOrder,
      nIterations,
      mesh,
      computeRegularStructure,
      meshBoundary);
    return;
  }

  /*!
   * Run the contour tree analysis. This helper function is used to
   * allow one to run the contour tree in a consistent fashion independent
   * of whether the data is 2D, 3D, or 3D_MC. This function just calls
   * Run2D, Run3D, or Run3D_MC depending on the type
   *
   *  computeRegularStructure : 0=Off, 1=full augmentation with all vertices
   *                            2=boundary augmentation using meshBoundary
   */
  template <typename FieldType, typename StorageType>
  void Run(const vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           std::vector<std::pair<std::string, vtkm::Float64>>& timings,
           contourtree_augmented::ContourTree& contourTree,
           contourtree_augmented::IdArrayType& sortOrder,
           vtkm::Id& nIterations,
           const vtkm::Id nRows,
           const vtkm::Id nCols,
           const vtkm::Id nSlices = 1,
           bool useMarchingCubes = false,
           unsigned int computeRegularStructure = 1)
  {
    using namespace vtkm::worklet::contourtree_augmented;
    // 2D Contour Tree
    if (nSlices == 1)
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_2D_Freudenthal<FieldType, StorageType> mesh(nRows, nCols);
      // Run the contour tree on the mesh
      RunContourTree(fieldArray,
                     timings,
                     contourTree,
                     sortOrder,
                     nIterations,
                     mesh,
                     computeRegularStructure,
                     mesh.GetMeshBoundaryExecutionObject());
      return;
    }
    // 3D Contour Tree using marching cubes
    else if (useMarchingCubes)
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_3D_MarchingCubes<FieldType, StorageType> mesh(nRows, nCols, nSlices);
      // Run the contour tree on the mesh
      RunContourTree(fieldArray,
                     timings,
                     contourTree,
                     sortOrder,
                     nIterations,
                     mesh,
                     computeRegularStructure,
                     mesh.GetMeshBoundaryExecutionObject());
      return;
    }
    // 3D Contour Tree with Freudenthal
    else
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_3D_Freudenthal<FieldType, StorageType> mesh(nRows, nCols, nSlices);
      // Run the contour tree on the mesh
      RunContourTree(fieldArray,
                     timings,
                     contourTree,
                     sortOrder,
                     nIterations,
                     mesh,
                     computeRegularStructure,
                     mesh.GetMeshBoundaryExecutionObject());
      return;
    }
  }


private:
  /*!
  *  Run the contour tree for the given mesh. This function implements the main steps for
  *  computing the contour tree after the mesh has been constructed using the approbrite
  *  contour tree mesh class.
  *
  *  meshBoundary : This parameter is generated by calling mesh.GetMeshBoundaryExecutionObject
  *                 For regular 2D/3D meshes this required no extra parameters, however, for a
  *                 ContourTreeMesh additional information about the block must be given. Rather
  *                 than generating the MeshBoundary descriptor here, we therefore, require it
  *                 as an input. The MeshBoundary is used to augment the contour tree with the
  *                 mesh boundary vertices. It is needed only if we want to augement by the
  *                 mesh boundary and computeRegularStructure is False (i.e., if we compute
  *                 the full regular strucuture this is not needed because all vertices
  *                 (including the boundary) will be addded to the tree anyways.
  *  computeRegularStructure : 0=Off, 1=full augmentation with all vertices
  *                            2=boundary augmentation using meshBoundary
  */
  template <typename FieldType,
            typename StorageType,
            typename MeshClass,
            typename MeshBoundaryClass>
  void RunContourTree(const vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
                      std::vector<std::pair<std::string, vtkm::Float64>>& timings,
                      contourtree_augmented::ContourTree& contourTree,
                      contourtree_augmented::IdArrayType& sortOrder,
                      vtkm::Id& nIterations,
                      MeshClass& mesh,
                      unsigned int computeRegularStructure,
                      const MeshBoundaryClass& meshBoundary)
  {
    using namespace vtkm::worklet::contourtree_augmented;
    // Stage 1: Load the data into the mesh. This is done in the Run() method above and accessible
    //          here via the mesh parameter. The actual data load is performed outside of the
    //          worklet in the example contour tree app (or whoever uses the worklet)

    // Stage 2 : Sort the data on the mesh to initialize sortIndex & indexReverse on the mesh
    // Start the timer for the mesh sort
    vtkm::cont::Timer timer;
    timer.Start();
    mesh.SortData(fieldArray);
    timings.push_back(std::pair<std::string, vtkm::Float64>("Sort Data", timer.GetElapsedTime()));
    timer.Start();

    // Stage 3: Assign every mesh vertex to a peak
    MeshExtrema extrema(mesh.nVertices);
    extrema.SetStarts(mesh, true);
    extrema.BuildRegularChains(true);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Join Tree Regular Chains", timer.GetElapsedTime()));
    timer.Start();

    // Stage 4: Identify join saddles & construct Active Join Graph
    MergeTree joinTree(mesh.nVertices, true);
    ActiveGraph joinGraph(true);
    joinGraph.Initialise(mesh, extrema);
    timings.push_back(std::pair<std::string, vtkm::Float64>("Join Tree Initialize Active Graph",
                                                            timer.GetElapsedTime()));

#ifdef DEBUG_PRINT
    joinGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
#endif
    timer.Start();

    // Stage 5: Compute Join Tree Hyperarcs from Active Join Graph
    joinGraph.MakeMergeTree(joinTree, extrema);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Join Tree Compute", timer.GetElapsedTime()));
#ifdef DEBUG_PRINT
    joinTree.DebugPrint("Join tree Computed", __FILE__, __LINE__);
    joinTree.DebugPrintTree("Join tree", __FILE__, __LINE__, mesh);
#endif
    timer.Start();

    // Stage 6: Assign every mesh vertex to a pit
    extrema.SetStarts(mesh, false);
    extrema.BuildRegularChains(false);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Spit Tree Regular Chains", timer.GetElapsedTime()));
    timer.Start();

    // Stage 7:     Identify split saddles & construct Active Split Graph
    MergeTree splitTree(mesh.nVertices, false);
    ActiveGraph splitGraph(false);
    splitGraph.Initialise(mesh, extrema);
    timings.push_back(std::pair<std::string, vtkm::Float64>("Split Tree Initialize Active Graph",
                                                            timer.GetElapsedTime()));
#ifdef DEBUG_PRINT
    splitGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
#endif
    timer.Start();

    // Stage 8: Compute Split Tree Hyperarcs from Active Split Graph
    splitGraph.MakeMergeTree(splitTree, extrema);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Split Tree Compute", timer.GetElapsedTime()));
#ifdef DEBUG_PRINT
    splitTree.DebugPrint("Split tree Computed", __FILE__, __LINE__);
    // Debug split and join tree
    joinTree.DebugPrintTree("Join tree", __FILE__, __LINE__, mesh);
    splitTree.DebugPrintTree("Split tree", __FILE__, __LINE__, mesh);
#endif
    timer.Start();

    // Stage 9: Join & Split Tree are Augmented, then combined to construct Contour Tree
    contourTree.Init(mesh.nVertices);
    ContourTreeMaker treeMaker(contourTree, joinTree, splitTree);
    // 9.1 First we compute the hyper- and super- structure
    treeMaker.ComputeHyperAndSuperStructure();
    timings.push_back(std::pair<std::string, vtkm::Float64>(
      "Contour Tree Hyper and Super Structure", timer.GetElapsedTime()));
    timer.Start();

    // 9.2 Then we compute the regular structure
    if (computeRegularStructure == 1) // augment with all vertices
    {
      treeMaker.ComputeRegularStructure(extrema);
      timings.push_back(std::pair<std::string, vtkm::Float64>("Contour Tree Regular Structure",
                                                              timer.GetElapsedTime()));
    }
    else if (computeRegularStructure == 2) // augment by the mesh boundary
    {
      treeMaker.ComputeBoundaryRegularStructure(extrema, mesh, meshBoundary);
      timings.push_back(std::pair<std::string, vtkm::Float64>(
        "Contour Tree Boundary Regular Structure", timer.GetElapsedTime()));
    }

    // Collect the output data
    nIterations = treeMaker.nIterations;
    sortOrder = mesh.sortOrder;
    // ProcessContourTree::CollectSortedSuperarcs<DeviceAdapter>(contourTree, mesh.sortOrder, saddlePeak);
    // contourTree.SortedArcPrint(mesh.sortOrder);
    // contourTree.PrintDotSuperStructure();
  }
};

} // namespace vtkm
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ContourTreeUniformAugmented_h
