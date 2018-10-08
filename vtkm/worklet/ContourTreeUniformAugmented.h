//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
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

namespace vtkm
{
namespace worklet
{

class ContourTreePPP2
{
public:
  /*!
   * Run the contour tree analysis. This helper function is used to
   * allow one to run the contour tree in a consistent fashion independent
   * of whether the data is 2D, 3D, or 3D_MC. This function just calls
   * Run2D, Run3D, or Run3D_MC depending on the type
   */
  template <typename FieldType, typename StorageType, typename DeviceAdapter>
  void Run(const vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           std::vector<std::pair<std::string, vtkm::Float64>>& timings,
           contourtree_augmented::ContourTree& contourTree,
           contourtree_augmented::IdArrayType& sortOrder,
           vtkm::Id& nIterations,
           const DeviceAdapter& device,
           const vtkm::Id nRows,
           const vtkm::Id nCols,
           const vtkm::Id nSlices = 1,
           bool useMarchingCubes = false,
           bool computeRegularStructure = true)
  {
    using namespace vtkm::worklet::contourtree_augmented;
    // 2D Contour Tree
    if (nSlices == 1)
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_2D_Freudenthal<FieldType, StorageType> mesh(nRows, nCols);
      // Run the contour tree on the mesh
      return RunContourTree(fieldArray,
                            timings,
                            contourTree,
                            sortOrder,
                            nIterations,
                            device,
                            nRows,
                            nCols,
                            1,
                            mesh,
                            computeRegularStructure);
    }
    // 3D Contour Tree using marching cubes
    else if (useMarchingCubes)
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_3D_MarchingCubes<FieldType, StorageType> mesh(nRows, nCols, nSlices);
      // Run the contour tree on the mesh
      return RunContourTree(fieldArray,
                            timings,
                            contourTree,
                            sortOrder,
                            nIterations,
                            device,
                            nRows,
                            nCols,
                            nSlices,
                            mesh,
                            computeRegularStructure);
    }
    // 3D Contour Tree with Freudenthal
    else
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_3D_Freudenthal<FieldType, StorageType> mesh(nRows, nCols, nSlices);
      // Run the contour tree on the mesh
      return RunContourTree(fieldArray,
                            timings,
                            contourTree,
                            sortOrder,
                            nIterations,
                            device,
                            nRows,
                            nCols,
                            nSlices,
                            mesh,
                            computeRegularStructure);
    }
  }


private:
  /*!
  *  Run the contour tree for the given mesh. This function implements the main steps for
  *  computing the contour tree after the mesh has been constructed using the approbrite
  *  contour tree mesh class
  */
  template <typename FieldType, typename StorageType, typename DeviceAdapter, typename MeshClass>
  void RunContourTree(const vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
                      std::vector<std::pair<std::string, vtkm::Float64>>& timings,
                      contourtree_augmented::ContourTree& contourTree,
                      contourtree_augmented::IdArrayType& sortOrder,
                      vtkm::Id& nIterations,
                      const DeviceAdapter& device,
                      const vtkm::Id /*nRows*/,   // FIXME: Remove unused parameter?
                      const vtkm::Id /*nCols*/,   // FIXME: Remove unused parameter?
                      const vtkm::Id /*nSlices*/, // FIXME: Remove unused parameter?
                      MeshClass& mesh,
                      bool computeRegularStructure)
  {
    using namespace vtkm::worklet::contourtree_augmented;
    // Start the timer
    vtkm::cont::Timer<DeviceAdapter> totalTime;

    // Stage 1: Load the data into the mesh. This is done in the Run() method above and accessible
    //          here via the mesh parameter. The actual data load is performed outside of the
    //          worklet in the example contour tree app (or whoever uses the worklet)

    // Stage 2 : Sort the data on the mesh to initialize sortIndex & indexReverse on the mesh
    // Start the timer for the mesh sort
    vtkm::cont::Timer<DeviceAdapter> timer;
    mesh.SortData(device, fieldArray);
    timings.push_back(std::pair<std::string, vtkm::Float64>("Sort Data", timer.GetElapsedTime()));
    timer.Reset();

    // Stage 3: Assign every mesh vertex to a peak
    MeshExtrema extrema(device, mesh.nVertices);
    extrema.SetStarts(mesh, true);
    extrema.BuildRegularChains(true);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Join Tree Regular Chains", timer.GetElapsedTime()));
    timer.Reset();

    // Stage 4: Identify join saddles & construct Active Join Graph
    MergeTree joinTree(device, mesh.nVertices, true);
    ActiveGraph joinGraph(device, true);
    joinGraph.Initialise(mesh, extrema);
    timings.push_back(std::pair<std::string, vtkm::Float64>("Join Tree Initialize Active Graph",
                                                            timer.GetElapsedTime()));

#ifdef DEBUG_PRINT
    joinGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
    joinGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
#endif
    timer.Reset();

    // Stage 5: Compute Join Tree Hyperarcs from Active Join Graph
    joinGraph.MakeMergeTree(joinTree, extrema);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Join Tree Compute", timer.GetElapsedTime()));
#ifdef DEBUG_PRINT
    joinTree.DebugPrint("Join tree Computed", __FILE__, __LINE__);
    joinTree.DebugPrintTree("Join tree", __FILE__, __LINE__, mesh);
#endif
    timer.Reset();

    // Stage 6: Assign every mesh vertex to a pit
    extrema.SetStarts(mesh, false);
    extrema.BuildRegularChains(false);
    timings.push_back(
      std::pair<std::string, vtkm::Float64>("Spit Tree Regular Chains", timer.GetElapsedTime()));
    timer.Reset();

    // Stage 7:     Identify split saddles & construct Active Split Graph
    MergeTree splitTree(device, mesh.nVertices, false);
    ActiveGraph splitGraph(device, false);
    splitGraph.Initialise(mesh, extrema);
    timings.push_back(std::pair<std::string, vtkm::Float64>("Split Tree Initialize Active Graph",
                                                            timer.GetElapsedTime()));
#ifdef DEBUG_PRINT
    splitGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
#endif
    timer.Reset();

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
    timer.Reset();

    // Stage 9: Join & Split Tree are Augmented, then combined to construct Contour Tree
    contourTree.Init(device, mesh.nVertices);
    ContourTreeMaker treeMaker(device, contourTree, joinTree, splitTree);
    // 9.1 First we compute the hyper- and super- structure
    treeMaker.ComputeHyperAndSuperStructure();
    timings.push_back(std::pair<std::string, vtkm::Float64>(
      "Contour Tree Hyper and Super Structure", timer.GetElapsedTime()));
    timer.Reset();

    // 9.2 Then we compute the regular structure
    if (computeRegularStructure)
    {
      treeMaker.ComputeRegularStructure(extrema);
      timings.push_back(std::pair<std::string, vtkm::Float64>("Contour Tree Regular Structure",
                                                              timer.GetElapsedTime()));
      timer.Reset();
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
